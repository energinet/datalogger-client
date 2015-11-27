/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2012 LIAB ApS <info@liab.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "xml_cntn_tx.h"
#include "xml_cntn_trx.h"
#include "xml_cntn_gen.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>
#include <sys/socket.h>

void* xml_cntn_tx_loop(void *parm);


struct  xml_cntn_tx*  xml_cntn_tx_create(int client_fd, int dbglev)
{
    struct xml_cntn_tx *new = malloc(sizeof(*new));
    int retval = 0;
	assert(new);
  
	if((retval = pthread_mutex_init(&new->tx_mutex_buf, NULL))!=0){
		PRINT_ERR("Failure in pthread_mutex_init tx_mutex_buf %p: %s ", new, strerror(retval));  
		return NULL;
	}

	if((retval = pthread_cond_init(&new->tx_cond, NULL))!=0){
		PRINT_ERR("Failure in pthread_cond_init %p: %s ", new, strerror(retval));  
		return NULL;
	}

    new->tx_doc1 = xml_doc_create(1024*200, dbglev);
	assert(new->tx_doc1);
    new->tx_doc2 = xml_doc_create(1024*200, dbglev);
	assert(new->tx_doc2);
	
    new->tx_doc = new->tx_doc1;
    new->tx_doc_buf = new->tx_doc2;

    new->tx_doc1->first = xml_item_create(new->tx_doc1, "trnf");
	assert(new->tx_doc1->first);

    new->tx_doc2->first = xml_item_create(new->tx_doc2, "trnf");
	assert(new->tx_doc2->first);

	new->cnfd = xml_cntn_fd_create(&new->fd);

	if(client_fd)
		xml_cntn_fd_open(new->cnfd, client_fd);

	new->reset_on_full = 1;

	new->run  =1 ;
	new->active = 0;
	new->dbglev  = dbglev ;

	/* start the loop */
	pthread_mutex_lock(&new->tx_mutex_buf);

    if((retval = pthread_create(&new->tx_thread, NULL, xml_cntn_tx_loop, new))<0){  
		PRINT_ERR("Failure starting loop in %p: %s ", new, strerror(retval));  
		return NULL;
    }  
	
	pthread_mutex_lock(&new->tx_mutex_buf);
	pthread_mutex_unlock(&new->tx_mutex_buf);

	PRINT_DBG(dbglev, "created %p, &new->tx_cond %p, &new->tx_mutex_buf %p %s", new, &new->tx_cond, &new->tx_mutex_buf, (new->active)?"active":"INACTIVE" );
	assert(new->active);
	
    return new;

}

void xml_cntn_tx_delete(struct  xml_cntn_tx* txcn)
{
	int retval;
    void* retptr = NULL;
	
    retval = pthread_mutex_lock(&txcn->tx_mutex_buf);
    PRINT_DBG(txcn->dbglev, "locked tx_mutex %d (%p)", retval ,txcn); 
	txcn->run = 0; 
    PRINT_DBG(txcn->dbglev, "txcn->run = 0(%p)", txcn); 
	PRINT_DBG(txcn->dbglev, "&txcn->tx_cond %p", &txcn->tx_cond); 
    pthread_cond_broadcast(&txcn->tx_cond); //Note: crashed here
    PRINT_DBG(txcn->dbglev, "send cond tx_cond");
    pthread_mutex_unlock(&txcn->tx_mutex_buf);
    PRINT_DBG(txcn->dbglev, "unlock cond tx_cond");

	xml_cntn_fd_delete(txcn->cnfd);

	retval = pthread_join(txcn->tx_thread, &retptr);

	PRINT_ERR("pthread_join tx_thread returned %d in %p tx: %s", retval, txcn, strerror(-retval));

	PRINT_DBG(txcn->dbglev, "stoped %p tx", txcn);
	
	pthread_cond_destroy(&txcn->tx_cond);
    pthread_mutex_destroy(&txcn->tx_mutex_buf);
	pthread_mutexattr_destroy(&txcn->tx_mutex_attr);

    xml_doc_delete(txcn->tx_doc1);
    xml_doc_delete(txcn->tx_doc2);
	
	free(txcn);

}



void xml_cntn_tx_doc_reset(struct xml_cntn_tx *txcn, struct xml_doc *tx_doc)
{
	pthread_mutex_lock(&txcn->tx_mutex_buf);
	
    xml_doc_reset(tx_doc);

    tx_doc->first = xml_item_create(tx_doc, "trnf");

	pthread_mutex_unlock(&txcn->tx_mutex_buf);
}



void xml_cntn_tx_flip_(struct xml_cntn_tx *txcn)
{
	PRINT_DBG((txcn->dbglev >= 2),"action flipped (%p)",txcn );
	
	if(txcn->tx_doc == txcn->tx_doc1){
		txcn->tx_doc = txcn->tx_doc2;
		txcn->tx_doc_buf = txcn->tx_doc1;
	} else {
		txcn->tx_doc = txcn->tx_doc1;
		txcn->tx_doc_buf = txcn->tx_doc2;
	}	
}


int xml_cntn_tx_flip(struct xml_cntn_tx *txcn, int block)
{
	int retval = 0;

	pthread_mutex_lock(&txcn->tx_mutex_buf);
	
	if(block){
		while(!xml_doc_roothasitems(txcn->tx_doc_buf)&&(txcn->run==1)){
			if((retval = pthread_cond_wait(&txcn->tx_cond, &txcn->tx_mutex_buf))!=0){
				PRINT_ERR("ERROR-> pthread_cond_wait failed: %s (%d) in %p",strerror(retval), retval,  txcn );
				return -retval;
			}
		}
	} else {
		if(!xml_doc_roothasitems(txcn->tx_doc_buf)){
			goto out;
		}		
	}

	if(!txcn->run){
		PRINT_DBG(txcn->dbglev,"NO action (%p)",txcn );
		retval = -1;
		goto out;
	}

	xml_cntn_tx_flip_(txcn);
  out:

	pthread_mutex_unlock(&txcn->tx_mutex_buf);

	return retval;
}


int xml_cntn_tx_dotx(struct xml_cntn_tx *txcn, int block)
{
	int retval = 0;
	
	if((retval = xml_cntn_tx_flip(txcn, block))!=0)
		return retval;

	
	if((retval = spxml_cntn_trx_tx(txcn->tx_doc, txcn->fd))<0){
		PRINT_ERR("spxml_cntn_trx_tx failed (%p): %d", txcn, retval);
		return retval;
	}

	xml_cntn_tx_doc_reset(txcn, txcn->tx_doc);

	return 0;
}





void* xml_cntn_tx_loop(void *parm)
{
    struct xml_cntn_tx *txcn = (struct xml_cntn_tx *)parm;
    int retval;

	

	PRINT_DBG(txcn->dbglev ,"started (%p)",txcn );
	txcn->active = 1;
	pthread_mutex_unlock(&txcn->tx_mutex_buf);

	while(xml_cntn_fd_wait(txcn->cnfd)==0){
		
		while(txcn->run){
			PRINT_DBG(txcn->dbglev,"action loop start (%p)",txcn );
			
			if((retval = xml_cntn_tx_dotx(txcn, 1))!=0)
				PRINT_DBG(txcn->dbglev,"xml_cntn_tx_dotx failed (%p): %d",txcn,  retval);
			
			if((retval = pthread_mutex_lock(&txcn->tx_mutex_buf))!=0){
				PRINT_ERR("Failure in mutex: %d -------------------------------------------------", retval);  
				goto out;
			}
			
			pthread_cond_broadcast(&txcn->tx_cond);
			
			pthread_mutex_unlock(&txcn->tx_mutex_buf);
			
		}
	}

  out:

	txcn->active = 0;

    PRINT_DBG(txcn->dbglev,"ended(%p)",txcn);
	
	pthread_exit(NULL);

	return NULL;
	
}




struct xml_doc* xml_cntn_tx_add_reserve(struct xml_cntn_tx *txcn)
{
    struct xml_doc* doc = NULL;
	
    if(!txcn->run){
		PRINT_ERR("Failure cntn not running"); 
		return NULL;
    }

    if(pthread_mutex_lock(&txcn->tx_mutex_buf)!=0){
		PRINT_ERR("Failure in mutex");  
		return NULL; 
    }

    doc = txcn->tx_doc_buf;

    if(xml_doc_space_remain(doc)<0){
		if(txcn->reset_on_full){
			xml_cntn_tx_doc_reset(txcn, doc);
			PRINT_ERR("doc is full (now reset)");  
		}else{
			pthread_cond_broadcast(&txcn->tx_cond);
			pthread_mutex_unlock(&txcn->tx_mutex_buf);
			PRINT_ERR("doc is full");  
			return NULL;
		}
    }
    
    return doc;
}

void xml_cntn_tx_add_commit(struct xml_cntn_tx *txcn){

	if(pthread_mutex_trylock(&txcn->tx_mutex_buf)==0)
		PRINT_ERR("doc mutex was not locked");  
	
	pthread_cond_broadcast(&txcn->tx_cond);

    pthread_mutex_unlock(&txcn->tx_mutex_buf);

	PRINT_DBG(txcn->dbglev , "%p commited tx", txcn);

}


int xml_cntn_tx_isactive(struct xml_cntn_tx *txcn)
{
	int active  = 0;
	
	if(pthread_mutex_trylock(&txcn->tx_mutex_buf))
		return 1;

	active |= xml_doc_roothasitems(txcn->tx_doc1);
	active |= xml_doc_roothasitems(txcn->tx_doc2);

	pthread_mutex_unlock(&txcn->tx_mutex_buf);

	return active;
}

int xml_cntn_tx_open(struct xml_cntn_tx *txcn, int client_fd)
{
	if(xml_cntn_fd_isactive(txcn->cnfd)){
		PRINT_DBG(txcn->dbglev , "%p commited tx", txcn);
		return -1;
	}

	txcn->run = 1;

	return xml_cntn_fd_open(txcn->cnfd, client_fd);
	
}


int xml_cntn_tx_close(struct xml_cntn_tx *txcn)
{

	pthread_mutex_lock(&txcn->tx_mutex_buf);
	txcn->run = 0; 
    pthread_cond_broadcast(&txcn->tx_cond); 
    pthread_mutex_unlock(&txcn->tx_mutex_buf);

	xml_cntn_fd_close(txcn->cnfd);

	return 0;
}
