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

#include "xml_cntn.h"

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

#include "xml_cntn_trx.h"


void* spxml_cntn_rx_loop_(void *parm);

struct  spxml_cntn*  spxml_cntn_create(int client_fd, struct tag_handler *tag_handlers, void *userdata,  int dbglev)
{
    struct spxml_cntn *new = malloc(sizeof(*new));
    int retval = 0;
	assert(new);

	new->txobj =  xml_cntn_tx_create(client_fd,dbglev);
	assert(new->txobj);

    new->rx_doc = xml_doc_create(1024*200, dbglev);
	assert(new->rx_doc);

    new->rx_stack  = xml_stack_create(30,new->rx_doc, dbglev );
	assert(new->rx_stack);

	if((retval = pthread_mutex_init(&new->mutex, NULL))!=0){
		PRINT_ERR("Failure in pthread_mutex_init tx_mutex_buf %p: %s ", new, strerror(retval));  
		return NULL;
	}

	new->cnfd = xml_cntn_fd_create(&new->fd);

	if(client_fd)
		xml_cntn_fd_open(new->cnfd, client_fd);

	new->run  =1 ;
    new->reset_on_full = 1;

	new->subcs = NULL;
	new->active = 0;

    new->tag_handlers = tag_handlers;
    new->userdata = userdata;
    new->dbglev  = dbglev;
	
	/* start loop */
	pthread_mutex_lock(&new->mutex);
	
	if((retval = pthread_create(&new->rx_thread, NULL, spxml_cntn_rx_loop_, new))<0){  
		PRINT_ERR("Failure starting loop in %p: %s ", new, strerror(retval));  
		return NULL;
	}  
	
	pthread_mutex_lock(&new->mutex);
	pthread_mutex_unlock(&new->mutex);

	PRINT_DBG(dbglev, "created %p,   --------------------------------------->\n", new) ;
	
    return new;

}

void spxml_cntn_delete(struct spxml_cntn *cntn)
{
    void* retptr = NULL;
    int retval = 0;
    if(!cntn)
		return;

	xml_cntn_tx_delete(cntn->txobj);

	xml_cntn_fd_delete(cntn->cnfd);

    retval = pthread_join(cntn->rx_thread, &retptr);
    if(retval < 0)
		PRINT_ERR("pthread_join rx_thread returned %d in %p rx: %s", retval, cntn, strerror(-retval));
	
	if(cntn->fd>=0){
		close(cntn->fd);
		PRINT_DBG(cntn->dbglev, "fd closed %p\n", cntn);
	}


    xml_stack_delete(cntn->rx_stack);
    xml_doc_delete(cntn->rx_doc);

    PRINT_DBG(cntn->dbglev, "%p deleted\n", cntn);

	pthread_mutex_destroy(&cntn->mutex);

    free(cntn);

}


int spxml_cntn_isactive(struct spxml_cntn *cntn)
{
	int active = 0;
	active |= xml_cntn_tx_isactive(cntn->txobj);
	active |= xml_cntn_fd_isactive(cntn->cnfd);

	return active;
}


struct xml_doc* spxml_cntn_tx_add_reserve(struct spxml_cntn *cntn) 
{ 
	return xml_cntn_tx_add_reserve(cntn->txobj);
} 
	 
void spxml_cntn_tx_add_commit(struct spxml_cntn *cntn){ 
	 
	xml_cntn_tx_add_commit(cntn->txobj);

} 





 int spxml_cntn_tx_add_err(struct spxml_cntn *cntn, const char *errstr, const char *seq) 
 { 
	 struct xml_item *item = NULL;
	 struct xml_doc* doc = spxml_cntn_tx_add_reserve(cntn);

	 if(!doc){
		 return -EFAULT;
	 }

    item = xml_item_create(doc, "err");
	
    xml_item_add_text(doc, item, xml_doc_text_dup(doc, errstr, 0),0);

    if(seq)
		xml_item_add_attr(doc, item, "seq",xml_doc_text_dup(doc, seq, 0));
	
    xml_item_add_child(doc, doc->first, item);
	
	spxml_cntn_tx_add_commit(cntn);
	
    return 0;

 } 


int spxml_cntn_run(struct spxml_cntn *cntn, struct xml_item *item)
{
    struct tag_handler *tag_handlers = cntn->tag_handlers;
    int i = 0;

    while(tag_handlers[i].function){
		if(tag_handlers[i].tag[0] == '*')
			return tag_handlers[i].function(cntn, item );
		if(strcmp(tag_handlers[i].tag, item->name)==0){
			return tag_handlers[i].function(cntn, item );
		}
		i++;
    }
	
    if(strcmp(item->name,"exit")==0)
		return -ESHUTDOWN;
	
    return 0;

}


int spxml_cntn_run_list(struct spxml_cntn *cntn, struct xml_item *list)
{
    int retval = 0;
    
    struct xml_item *item = list;
    while(item){
		retval = spxml_cntn_run(cntn, item);
		if(retval<0)
			break;
		item = item->next;
    }

    return retval;
    
}


void* spxml_cntn_rx_loop_(void *parm)
{
    struct spxml_cntn *cntn = (struct spxml_cntn *)parm;

	cntn->active = 1;
    PRINT_DBG(cntn->dbglev,"starting (%p)",cntn);

	pthread_mutex_unlock(&cntn->mutex);

	while(xml_cntn_fd_wait(cntn->cnfd)==0){
		while(cntn->run){
			int retval = 0;
			xml_stack_reset(cntn->rx_stack);		
			retval = spxml_cntn_trx_read_doc(cntn->fd, cntn->rx_stack, &cntn->run, 0, cntn->dbglev);
			
			if(retval == -3){
				/*PRINT_ERR("ERR > spxml_cntn_trx_read_doc returned %d. Exiting ", retval);*/
				break;
			}
			
			if(retval <-1){
				char outbuf[1024*1000];
				snprintf(outbuf,sizeof(outbuf), "Parse error at line %d:%s",
						 (int)XML_GetCurrentLineNumber( cntn->rx_stack->parser),
						 XML_ErrorString(XML_GetErrorCode( cntn->rx_stack->parser)));
				spxml_cntn_tx_add_err(cntn, outbuf, NULL);
			} else {
				struct xml_item *trnf = cntn->rx_doc->first;
				retval = spxml_cntn_run_list(cntn, trnf->childs);
				if(retval < 0)
					break;
			}

		}
		

    }

    PRINT_DBG(cntn->dbglev,"ended (%p)",cntn);
	cntn->active = 0;

	pthread_exit(NULL);

}

void spxml_cntn_publs_add(struct spxml_cntn *cntn, const void *publisher)
{
	cntn->subcs = xmc_cntn_sub_list_add(cntn->subcs, xmc_cntn_sub_create(publisher));
}

int spxml_cntn_publs_sub(struct spxml_cntn *cntn, const void *publisher)
{
	if(cntn->run)
		return  xmc_cntn_sub_list_isin(cntn->subcs, publisher);
	return 0;
}




struct spxml_cntn_list *spxml_cntn_list_create(int size, struct tag_handler *tag_handlers, void *userdata, int dbglev )
{
	int i;
	struct spxml_cntn_list *list = malloc(sizeof(*list));
	assert(list);

	list->size = size;

	list->cntns = (struct spxml_cntn**) malloc(sizeof(struct spxml_cntn*)*size);
	assert(list->cntns);
	
	memset(list->cntns, 0, sizeof(struct spxml_cntn*)*size);
	
	for(i= 0; i < size; i ++){
		list->cntns[i] = spxml_cntn_create(-1,tag_handlers, userdata, dbglev);
	}

	return list;

}


void spxml_cntn_list_delete(struct spxml_cntn_list *list)
{
	int i ;
	
	for(i = 0; i < list->size; i++){
		struct spxml_cntn *cntn = list->cntns[i];
		spxml_cntn_close(cntn);
		spxml_cntn_delete(cntn);
	}
	
	free(list->cntns);
	free(list);
	
}


int spxml_cntn_list_activecnt(struct spxml_cntn_list *list)
{
	int i, count = 0 ;
	
	for(i = 0; i < list->size; i++){
		struct spxml_cntn *cntn = list->cntns[i];
		if(cntn!=NULL){
			if(spxml_cntn_isactive(cntn)){
				count++;
			}
		}
	}
	
	return count;
	
}

int spxml_cntn_list_inactivecnt(struct spxml_cntn_list *list)
{
	int i, count = 0 ;
	
	for(i = 0; i < list->size; i++){
		struct spxml_cntn *cntn = list->cntns[i];
		if(cntn!=NULL){
			if(!spxml_cntn_isactive(cntn)){
				count++;
			}
		}
	}
	
	return count;
	
}




int spxml_cntn_list_open(struct spxml_cntn_list *list, int client_fd)
{
	int i;
	
	for(i = 0; i < list->size; i++){
		struct spxml_cntn *cntn = list->cntns[i];
		
		if(!spxml_cntn_isactive(cntn)){
			spxml_cntn_close(cntn);
			spxml_cntn_open(cntn, client_fd);
			return i;
		} else {
		}
	}

	return -1;
}


int spxml_cntn_list_clean(struct spxml_cntn_list *list)
{
	int i;
	int count = 0;
	for(i = 0; i < list->size; i++){
		struct spxml_cntn *cntn = list->cntns[i];
		if(cntn!=NULL){
			if(!spxml_cntn_isactive(cntn)){
				spxml_cntn_delete(cntn);
				list->cntns[i] = NULL;
				count++;
			}
		}
	}

	return count;

}


struct spxml_cntn *spxml_cntn_list_next(struct spxml_cntn_list *list, int *indexp)
{
	int index = *indexp;

	while(index < list->size){
		struct spxml_cntn *cntn = list->cntns[index++];
			if(spxml_cntn_isactive(cntn)){
				*indexp = index;
				return cntn;
			}
	}

	*indexp = index;
	return NULL;
	
}


int spxml_cntn_open(struct spxml_cntn *cntn, int client_fd)
{
	if(xml_cntn_fd_isactive(cntn->cnfd)||xml_cntn_tx_isactive(cntn->txobj)){
		PRINT_DBG(cntn->dbglev , "%p is active", cntn);
		return -1;
	}

	xml_cntn_tx_open(cntn->txobj, client_fd);

	cntn->run = 1;
	
	xml_cntn_fd_open(cntn->cnfd, client_fd);

	return 0;


}


int spxml_cntn_close(struct spxml_cntn *cntn)
{

	cntn->run = 0; 

	xml_cntn_fd_close(cntn->cnfd);

	xml_cntn_tx_close(cntn->txobj);

	xmc_cntn_sub_delete(cntn->subcs);

	cntn->subcs = NULL;

	

	return 0;
}



int spxml_cntn_send(struct spxml_cntn *cntn, struct xml_item *src, const char *seq)
{
    struct xml_item *item = NULL;
    struct xml_doc* doc = NULL;
    int retval = 0;
    
    if(!cntn)
	return -EFAULT;
    
    if(cntn->run==0)
	return -EFAULT;

    doc = spxml_cntn_tx_add_reserve(cntn);

    item = xml_item_create_copy(doc, src, 1);
	
    if(!item){
	PRINT_ERR("failed to create xml item for event");
	retval = -EFAULT;
	goto out;
    }

    if(seq){
	xml_item_add_attr(doc, item, "seq", xml_doc_text_dup(doc, seq,0));
    }    
	
    if(!xml_item_add_child(doc, doc->first, item)){
	PRINT_ERR("could not add child");
	retval = -EFAULT;
	goto out;
    }
    
  out:

    spxml_cntn_tx_add_commit(cntn);

    return retval;

}
