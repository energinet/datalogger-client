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

#include "xmlcntn.h"

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


void* spxml_cntn_rx_loop(void *parm);
void* spxml_cntn_tx_loop(void *parm);
void spxml_cntn_tx_doc_reset(struct xml_doc *tx_doc);

struct  spxml_cntn*  spxml_cntn_create(int client_fd, struct tag_handler *tag_handlers, void *userdata, int dbglev)
{
    struct spxml_cntn *new = malloc(sizeof(*new));
    int retval = 0;

    PRINT_DBG(dbglev, "creating\n");

    pthread_cond_init(&new->tx_cond, NULL);
    pthread_mutex_init(&new->tx_mutex, NULL);
    pthread_mutex_init(&new->tx_mutex_buf, NULL);
    new->tx_doc1 = xml_doc_create(1024*200, dbglev);
    new->tx_doc2 = xml_doc_create(1024*200, dbglev);
    new->rx_stack  = xml_doc_parse_prep(30,1024*200, dbglev );
    new->rx_doc = new->rx_stack->doc;

    new->tx_doc = new->tx_doc1;
    new->tx_doc_buf = new->tx_doc2;

    new->tx_doc1->first = xml_item_create(new->tx_doc1, "trnf");
    new->tx_doc2->first = xml_item_create(new->tx_doc2, "trnf");

    new->fd = client_fd;

    new->next = NULL;
    new->run  =1 ;
    new->reset_on_full = 1;

    new->tag_handlers = tag_handlers;
    new->userdata = userdata;
    new->dbglev  = dbglev;

    if((retval = pthread_create(&new->tx_thread, NULL, spxml_cntn_tx_loop, new))<0){  
	PRINT_ERR("Failure starting loop in %p: %s ", new, strerror(retval));  
	return NULL;
    }  

    if((retval = pthread_create(&new->rx_thread, NULL, spxml_cntn_rx_loop, new))<0){  
	PRINT_ERR("Failure starting loop in %p: %s ", new, strerror(retval));  
	return NULL;
    }  


    PRINT_DBG(new->dbglev, "created\n");
    
    return new;

}





void spxml_cntn_delete(struct spxml_cntn *cntn)
{
    void* retptr = NULL;
    int retval = 0;
    if(!cntn)
	return;

    PRINT_DBG(cntn->dbglev, "deleting %p\n", cntn);
    
    cntn->run = 0;
    
  

    retval = pthread_mutex_trylock(&cntn->tx_mutex);
    PRINT_DBG(cntn->dbglev >=2, "locked tx_mutex %d\n", retval);
    pthread_cond_broadcast(&cntn->tx_cond);
    PRINT_DBG(cntn->dbglev >=2, "send cond tx_cond\n");
    pthread_mutex_unlock(&cntn->tx_mutex);
    PRINT_DBG(cntn->dbglev >=2, "unlock cond tx_cond\n");
    pthread_cond_destroy(&cntn->tx_cond);
    pthread_mutex_destroy(&cntn->tx_mutex);
    pthread_mutex_destroy(&cntn->tx_mutex_buf);
    
    if(cntn->fd>2)
	close(cntn->fd);



    PRINT_DBG(cntn->dbglev >=2, "stopping %p rx\n", cntn);

    retval = pthread_join(cntn->rx_thread, &retptr);
    if(retval < 0)
	PRINT_ERR("pthread_join returned %d in %p rx: %s", retval, cntn, strerror(-retval));

    PRINT_DBG(cntn->dbglev >=2, "stopping %p tx\n", cntn);

    retval = pthread_join(cntn->tx_thread, &retptr);
    if(retval < 0)
	PRINT_ERR("pthread_join returned %d in %p tx: %s", retval, cntn, strerror(-retval));

    PRINT_DBG(cntn->dbglev >=2, "stopping %p tx\n", cntn);

    xml_doc_delete(cntn->tx_doc1);
    xml_doc_delete(cntn->tx_doc2);
    xml_stack_delete(cntn->rx_stack);

    PRINT_DBG(cntn->dbglev, "%p deleted\n", cntn);

    spxml_cntn_delete(cntn->next);

    free(cntn);

}





struct spxml_cntn* spxml_cntn_add(struct spxml_cntn *list ,struct spxml_cntn *new)
{
    struct spxml_cntn *ptr = list;

    if(!ptr){
	return new;
    }
      
    while(ptr->next)
	ptr = ptr->next;
    
    ptr->next = new;
    
    return list;
}


struct spxml_cntn* spxml_cntn_rem(struct spxml_cntn *list ,struct spxml_cntn *rem)
{
    struct spxml_cntn *ptr = list;

    if(!rem){
	return list;
    }
    
    if(ptr == rem){
	struct spxml_cntn *next = ptr->next;
	ptr->next = NULL;
	return next;
    }

    while(ptr->next){
	if(ptr->next == rem){
	    struct spxml_cntn *found = ptr->next;
	    ptr->next = found->next;
	    found->next = NULL;
	    break;
	}
	ptr = ptr->next;
    }
    

    return list;
    
}





struct spxml_cntn* spxml_cntn_clean(struct spxml_cntn *list)
{
    struct spxml_cntn *ptr = list;

    while(ptr){
	if(!ptr->run){
	    list = spxml_cntn_rem(list ,ptr);
	    spxml_cntn_delete(ptr);
	}
	ptr = ptr->next;
    }

    return list;
    
}








int spxml_cntn_tx_flip(struct spxml_cntn *cntn)
{

  /*   if(pthread_mutex_trylock(&cntn->tx_mutex)!=0){ */
/* 	return 0; */
/*     } */

/*     fprintf(stderr, "flip size %d\n", xml_doc_space_remain(cntn->tx_doc_buf)); */
    
/*     if(!cntn->tx_doc->first->childs){ */
/* 	if(cntn->tx_doc == cntn->tx_doc1){ */
/* 	    cntn->tx_doc = cntn->tx_doc2; */
/* 	    cntn->tx_doc_buf = cntn->tx_doc1; */
/* 	} else { */
/* 	    cntn->tx_doc = cntn->tx_doc1; */
/* 	    cntn->tx_doc_buf = cntn->tx_doc2; */
/* 	} */
/*     } else { */
/* //	if(cntn->dbglev) */
/* 	fprintf(stderr, "no flip size %d\n", xml_doc_space_remain(cntn->tx_doc_buf)); */
/*     } */

    pthread_cond_broadcast(&cntn->tx_cond);
    /* pthread_mutex_unlock(&cntn->tx_mutex); */

    return 0;

}


struct xml_doc* spxml_cntn_tx_add_reserve(struct spxml_cntn *cntn)
{
    struct xml_doc* doc = NULL;

    if(!cntn->run){
	PRINT_ERR("Failure cntn not running"); 
	return NULL;
    }

    if(pthread_mutex_lock(&cntn->tx_mutex_buf)!=0){
	PRINT_ERR("Failure in mutex");  
	return NULL; 
    }

    doc = cntn->tx_doc_buf;

    if(xml_doc_space_remain(doc)<0){
	if(cntn->reset_on_full){
	    spxml_cntn_tx_doc_reset(doc);
	    PRINT_ERR("doc is full (now reset)");  
	}else{
	    spxml_cntn_tx_flip(cntn);
	    pthread_mutex_unlock(&cntn->tx_mutex_buf);
	    PRINT_ERR("doc is full");  
	    return NULL;
	}
    }
    
    return doc;
}

void spxml_cntn_tx_add_commit(struct spxml_cntn *cntn){

    spxml_cntn_tx_flip(cntn);

    pthread_mutex_unlock(&cntn->tx_mutex_buf);
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




/** LOOP */

void spxml_cntn_tx_doc_reset(struct xml_doc *tx_doc)
{
    xml_doc_reset(tx_doc);

    tx_doc->first = xml_item_create(tx_doc, "trnf");

}


int spxml_cntn_tx(struct spxml_cntn *cntn)
{
    char buf[1024*100];
    int retval = 0;
    int len;

    len = xml_doc_print(cntn->tx_doc, buf, sizeof(buf));
    
    len += snprintf(buf+len, sizeof(buf)-len, "\r\n");
    
    retval = write(cntn->fd, buf, len);

    if(len != retval){
	PRINT_DBG(cntn->dbglev,"len (%d) != retval (%d)", len, retval);
	return -1;
    }   
    PRINT_DBG(cntn->dbglev,"len %d", len);

    spxml_cntn_tx_doc_reset(cntn->tx_doc);
    
    return 0;
    
}


void* spxml_cntn_tx_loop(void *parm)
{
    struct spxml_cntn *cntn = (struct spxml_cntn *)parm;
    int retval;

    if(pthread_mutex_lock(&cntn->tx_mutex)!=0)
	return NULL;

    while(cntn->run){

	if(pthread_mutex_lock(&cntn->tx_mutex_buf)!=0){
	    PRINT_ERR("Failure in mutex");  
	    break;
	}

	if(!cntn->tx_doc_buf->first->childs){
	    if(retval = pthread_cond_wait(&cntn->tx_cond, &cntn->tx_mutex_buf)) 
		break;
	}

	if(cntn->tx_doc == cntn->tx_doc1){
	    cntn->tx_doc = cntn->tx_doc2;
	    cntn->tx_doc_buf = cntn->tx_doc1;
	} else {
	    cntn->tx_doc = cntn->tx_doc1;
	    cntn->tx_doc_buf = cntn->tx_doc2;
	}
	
	pthread_mutex_unlock(&cntn->tx_mutex_buf);
	

	if(!cntn->fd)
	    break;
	
	if(spxml_cntn_tx(cntn)<0)
	    break;

    }

    pthread_mutex_unlock(&cntn->tx_mutex);
    
    cntn->run = 0;

    if(cntn->fd>2){
	close(cntn->fd);
	cntn->fd = 0;
    }

    PRINT_DBG(cntn->dbglev,"ended");

    return NULL;
}


int spxml_cntn_run(struct spxml_cntn *cntn, struct xml_item *item)
{
    struct tag_handler *tag_handlers = cntn->tag_handlers;
    int i = 0;

    while(tag_handlers[i].function){
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

#define POLL_TIMEOUT 10000


ssize_t spxml_read(int fd, void *buf, size_t count, int *run, int dbglev)
{
    struct pollfd poll_st;
    int retval = 0;
    poll_st.fd = fd;    
    poll_st.events = POLLIN;
    
    do{
	retval = poll(&poll_st, 1, POLL_TIMEOUT);
	PRINT_DBG(dbglev >= 2, "poll %d\n", retval);

	if(retval<0){
	    PRINT_ERR("poll returned error: %s\n",strerror(errno)); 
	    return -1;
	} else if (*run!=1) {
	    PRINT_ERR("poll returned because run = %d\n",*run);
	    return 0;
	}
	
    } while (retval==0);

    retval = read(fd, buf, count);

    if(retval == 0){
	PRINT_DBG(dbglev >= 1,"EOF");
    } else if(retval < 0){
	PRINT_ERR("socket read error %s\n",strerror(errno)); 	
    } 

    return retval;
}


void* spxml_cntn_rx_loop(void *parm)
{
    struct spxml_cntn *cntn = (struct spxml_cntn *)parm;
    int rx_buf_size = 1024;
    char *rx_buffer = malloc(rx_buf_size+1);
    char *outbuf = malloc(1024*1000);
    int retval = 0, readsize = 0, size = 0;
    int ptr = 0, buf_ptr = 0;

    int rd_ptr = 0;
    int wrd_ptr = 0;

    while(cntn->run){
	struct xml_item *trnf = NULL;
	char *newline = NULL;
	int trnf_size = 0;
	int trnf_size_parse = 0;
	int retval = 0;
	
	while(retval >= 0){
	    readsize = 0;

	    if(rd_ptr == wrd_ptr){
		size = spxml_read(cntn->fd, rx_buffer+wrd_ptr, rx_buf_size-wrd_ptr, &cntn->run, cntn->dbglev);
		if(size <= 0)
		    goto out;
		wrd_ptr += size;
	    }
	    
	    rx_buffer[wrd_ptr] = '\0';

	    newline = strchr(rx_buffer+rd_ptr, '\r');
	    if(newline){
		newline[0] = '\0';
		trnf_size = newline-(rx_buffer+rd_ptr)+1;
		trnf_size_parse = trnf_size-1;
		if(newline[1]=='\n'){
		    trnf_size++;
		}

	    } else {
		trnf_size = wrd_ptr-rd_ptr;
		trnf_size_parse = trnf_size;
	    }
	    ptr = rd_ptr;
	    
	    retval = xml_doc_parse_step(cntn->rx_stack, rx_buffer+rd_ptr, trnf_size_parse);
	    
	    if(retval <0){
		rd_ptr += trnf_size;
		if(rd_ptr == wrd_ptr){
		    rd_ptr = 0;
		    wrd_ptr = 0;
		}
		break;
	    }
	    rd_ptr = 0;
	    wrd_ptr = 0;
	}

	if(retval <-1){
	    PRINT_ERR("ERR >--inbuf(%d) >'%s'<\n", retval, rx_buffer+ptr);
	    snprintf(rx_buffer,rx_buf_size, "Parse error at line %d:%s",
                (int)XML_GetCurrentLineNumber( cntn->rx_stack->parser),
                XML_ErrorString(XML_GetErrorCode( cntn->rx_stack->parser)));
	    spxml_cntn_tx_add_err(cntn, rx_buffer, NULL);
	} else {
	    trnf = cntn->rx_doc->first;
	    retval = spxml_cntn_run_list(cntn, trnf->childs);
	    xml_doc_print(cntn->rx_doc, outbuf, 1024*1000);
	    if(retval < 0)
		break;
	}
	xml_doc_parse_reset(cntn->rx_stack);
	
    }

  out:
    free(outbuf);
    free(rx_buffer);
    
    cntn->run = 0;

    if(cntn->fd>2){
	close(cntn->fd);
	cntn->fd = 0;
    }

    PRINT_DBG(cntn->dbglev," ended\n");

    return NULL;
}

