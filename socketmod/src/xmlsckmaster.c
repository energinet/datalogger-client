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

#include "module_base.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/time.h>
#include <assert.h>
#include <asocket_trx.h>
#include <asocket_server.h>
#include <netinet/tcp.h>
#include "xmlcntn.h"
#include "xmlcontdaem.h"

#define DEFAULT_PORT 6527

#define RXSEQBUFSIZE 64

struct sevtread{
    int received;
    unsigned long seq; 
    struct module_event* event;
    struct sevtread *next;
};

struct sevtread *sevtread_add(struct sevtread *list ,struct sevtread *new)
{
    struct sevtread *ptr = list;

    if(!ptr){
	return new;
    }
      
    while(ptr->next)
	ptr = ptr->next;
    
    ptr->next = new;
    
    return list;
}


struct sevtread *sevtread_rem(struct sevtread *list ,struct sevtread *rem)
{
    struct sevtread *ptr = list;

    if(!rem){
	return list;
    }
    
    if(ptr == rem){
	struct sevtread *next = ptr->next;
	ptr->next = NULL;
	return next;
    }

    while(ptr->next){
	if(ptr->next == rem){
	    struct sevtread *found = ptr->next;
	    ptr->next = found->next;
	    found->next = NULL;
	    break;
	}
	ptr = ptr->next;
    }
    return list;
    
}



 struct cntntxrx{ 
     unsigned long seq; 
     pthread_mutex_t mutex; 
     pthread_cond_t cond; 
     struct sevtread *list;   
 }; 

void cntntxrx_init(struct cntntxrx *txrx) 
 { 
     txrx->seq = 0;
     txrx->list = NULL;
     
     pthread_cond_init(&txrx->cond, NULL); 
     pthread_mutex_init(&txrx->mutex, NULL); 
    
 } 


void cntntxrx_deinit(struct cntntxrx *txrx) 
 { 
     pthread_cond_destroy(&txrx->cond);
     pthread_mutex_destroy(&txrx->mutex);
 }

unsigned long cntntxrx_txseq(struct cntntxrx *txrx, struct sevtread *sevtrd) 
{ 
    memset(sevtrd, 0, sizeof(*sevtrd));

    if(pthread_mutex_lock(&txrx->mutex)!=0) 
 	return -1; 
    
    sevtrd->seq = ++txrx->seq;
    
    txrx->list = sevtread_add(txrx->list ,sevtrd);
    
    pthread_mutex_unlock(&txrx->mutex); 

    return sevtrd->seq;
    
}


int cntntxrx_rxseq(struct cntntxrx *txrx, unsigned long seq, struct module_event* event) 
 { 
     struct sevtread *ptr;
     int found = 0;

//     fprintf(stderr, "received %lu\n", seq);

     if(pthread_mutex_lock(&txrx->mutex)!=0) 
	 return -1; 
     
     ptr = txrx->list;

     while(ptr){
	 if(ptr->seq == seq){
	     ptr->event = event;
	     ptr->received = 1;
	     found = 1;
	     break;
	 }
	 ptr = ptr->next;
     }

     pthread_cond_broadcast(&txrx->cond); 
     pthread_mutex_unlock(&txrx->mutex); 

     if(!found){
	 PRINT_ERR("seq not found %lu", seq);
	 module_event_delete(event);
     }
	 

     return found;
    
 } 

struct module_event* cntntxrx_rxwait(struct cntntxrx *txrx, struct sevtread *sevtrd) 
 { 

     struct timespec ts_timeout; 
     int retval = 0;

     clock_gettime(CLOCK_REALTIME, &ts_timeout); 

     ts_timeout.tv_sec += 5; 

     if(pthread_mutex_lock(&txrx->mutex)!=0) 
	 return NULL; 

     while(!sevtrd->received){
	 if(retval = pthread_cond_timedwait(&txrx->cond, &txrx->mutex, &ts_timeout)){
	     PRINT_ERR("pthread_cond_timedwait failed (seq %ul)", sevtrd->seq);
	     break;
	 }
     }
     
     txrx->list = sevtread_rem(txrx->list ,sevtrd);

     pthread_mutex_unlock(&txrx->mutex); 
     
     return sevtrd->event;
	 
} 

void cntntxrx_cancel(struct cntntxrx *txrx, struct sevtread *sevtrd) 
{
     if(pthread_mutex_lock(&txrx->mutex)!=0) 
	 return;

     txrx->list = sevtread_rem(txrx->list ,sevtrd);
     
     pthread_mutex_unlock(&txrx->mutex); 
     
}



struct sevent{
    char *name;
    struct event_type *event_type;
    pthread_mutex_t txrx_mutex;
    pthread_cond_t txrx_cond;
    struct sevent *next;
};



struct mastersoxml_object{
    struct module_base base;
    struct spxml_cntn *cntn;
    struct cntntxrx txrx;
    struct sevent *sevts;
};


struct mastersoxml_object* module_get_struct(struct module_base *base){
    return (struct mastersoxml_object*) base;
}

struct module_event *sevent_eread(struct event_type *event);
int sevent_ewrite(struct event_type *event, struct uni_data *data);

struct sevent *sevent_create(struct module_base *base, struct xml_item *item)
{
    const char *etype = xml_item_get_attr(item, "ename");
    const char *unit = xml_item_get_attr(item, "unit");
    const char *text = xml_item_get_text(item);
    struct sevent *new = malloc(sizeof(*new));
    assert(new);
    memset(new, 0, sizeof(*new));

    if(!etype){
	PRINT_ERR("No name for event type");
	goto err_out;
    }

    new->name = strdup(etype);

    new->event_type = event_type_create(base, new, etype, unit , text, base->flags);

    new->event_type->read  = sevent_eread;
    new->event_type->write = sevent_ewrite;

    base->event_types = event_type_add(base->event_types,new->event_type); 

    return new;

  err_out:
    
    free(new->name);
    free(new);
    
    return NULL;
    
}

struct sevent *sevent_delete(struct sevent *sevt)
{
    if(!sevt)
	return;

    sevent_delete(sevt->next);

    free(sevt->name);
    free(sevt);

}


struct sevent *sevent_add(struct sevent *list, struct sevent *new)
{
    struct sevent *ptr = list;

    if(!ptr){
	return new;
    }
      
    while(ptr->next)
	ptr = ptr->next;
    
    ptr->next = new;
    
    return list;    
}



struct sevent *sevent_search(struct sevent *list, const char *name)
{
    struct sevent *ptr = list;

    if(!name)
	return NULL;

    while(ptr){
	if(strcmp(ptr->name, name)==0)
	    return ptr;
	ptr = ptr->next;
    }
    
    return NULL;

}


int sevent_send_event(struct sevent *sevt, struct uni_data *data, struct timeval *time)
{
    
    

    return module_base_send_event(module_event_create(sevt->event_type, data, time));


}

struct module_event *sevent_eread(struct event_type *event)
{
    struct sevtread sevtrd;
    struct mastersoxml_object *this = module_get_struct(event->base);
    unsigned long seq = cntntxrx_txseq(&this->txrx, &sevtrd);
    struct xml_doc* doc = spxml_cntn_tx_add_reserve(this->cntn);
    struct sevent *sevt = (struct sevent *)event->objdata;
    struct xml_item *item = NULL;

    if(!doc){
	goto err_out;
    }

    item = xml_item_create(doc, "rd");
    if(!item){
	PRINT_ERR("failed to create xml item for read request");
	goto err_out;
    }

    xml_item_add_attr(doc, item, "seq", xml_doc_text_printf(doc, "%ul", seq));

    xml_item_add_attr(doc, item, "ename" , sevt->name);

    if(!xml_item_add_child(doc, doc->first, item)){
	PRINT_ERR("could not add child");
	goto err_out;
    }
    
    spxml_cntn_tx_add_commit(this->cntn);
 
    return cntntxrx_rxwait(&this->txrx, &sevtrd);

  err_out:
    spxml_cntn_tx_add_commit(this->cntn);
    cntntxrx_cancel(&this->txrx, &sevtrd);

    return NULL;
    

}

int sevent_ewrite(struct event_type *event, struct uni_data *data)
{
    struct sevent *sevt = (struct sevent *)event->objdata;
    struct mastersoxml_object *this = module_get_struct(event->base); 
    struct xml_doc* doc = spxml_cntn_tx_add_reserve(this->cntn);
    
     struct xml_item *item = NULL;
    int retval = 0;

    if(!doc){
	return -EFAULT;
    }

    item = xml_item_create(doc, "wrt");
    if(!item){
	PRINT_ERR("failed to create xml item for write request");
	retval = -EFAULT;
	goto out;
    }

    xml_item_add_attr(doc, item, "ename" , sevt->name);

    if(!xml_item_add_child(doc, item, uni_data_xml_item(data, doc))){
	PRINT_ERR("could not create data");
	retval = -EFAULT;
	goto out;
    }

    if(!xml_item_add_child(doc, doc->first, item)){
	PRINT_ERR("could not add child");
	retval = -EFAULT;
	goto out;
    }

  out:

    spxml_cntn_tx_add_commit(this->cntn);




    return retval;
    

}


int tag_fin(struct spxml_cntn *cntn, struct xml_item *item )
{
    struct mastersoxml_object *this = (struct mastersoxml_object *)cntn->userdata;
    struct module_base* base = &this->base;
    struct sevent *sevt;
    const char *seq_attr = xml_item_get_attr(item, "seq");

    if(seq_attr){
	unsigned long seq; 
	sscanf(seq_attr, "%lu", &seq);
	cntntxrx_rxseq(&this->txrx, seq, NULL);
	fprintf(stderr, "received fin seq %lu (%s)\n", seq, seq_attr);
    } else {
	fprintf(stderr, "received fin with no seq\n");
    }

}





int tag_etype(struct spxml_cntn *cntn, struct xml_item *item )
{
    struct mastersoxml_object *this = (struct mastersoxml_object *)cntn->userdata;
    struct module_base* base = &this->base;
    struct sevent *sevt;
    const char *etype_attr = xml_item_get_attr(item, "ename");
    const char *seq_attr = xml_item_get_attr(item, "seq");

    PRINT_MVB(base, "received %s", etype_attr);

    sevt =  sevent_create(base, item);

    this->sevts = sevent_add(this->sevts, sevt);

    if(seq_attr){
	unsigned long seq; 
	sscanf(seq_attr, "%lu", &seq);
	cntntxrx_rxseq(&this->txrx, seq, NULL);
	fprintf(stderr, "received seq %lu\n", seq);
    } 

    return 0;
}


int tag_upd(struct spxml_cntn *cntn, struct xml_item *item )
{
    struct mastersoxml_object *this = (struct mastersoxml_object *)cntn->userdata;
    struct module_base* base = &this->base;
    struct sevent *sevt = NULL;
    const char *etype_attr = xml_item_get_attr(item, "ename");
    const char *time_attr = xml_item_get_attr(item, "time");
    const char *seq_attr = xml_item_get_attr(item, "seq");
    struct timeval time;

//    PRINT_MVB(base, "received, event %s time %s",etype_attr, time_attr);

    sevt = sevent_search(this->sevts, etype_attr);

    if(!sevt){
	return 0;
    }

//     PRINT_MVB(base, "Found %s",sevt->name);

    struct xml_item *data_item = xml_item_get_child(item, NULL, "data");
    
    if(!data_item){
	fprintf(stderr, "no data etype %s", etype_attr);
	goto err_out;
    }
    
    struct uni_data *data = uni_data_create_from_xmli(data_item);

    if(!data_item){
	fprintf(stderr, "error reading data from %s", etype_attr);
	goto err_out;
    }
    
    if(module_util_xml_to_time(&time, time_attr)!=0)
	goto err_out;

    if(seq_attr){
	unsigned long seq; 
	sscanf(seq_attr, "%lu", &seq);
	cntntxrx_rxseq(&this->txrx, seq, module_event_create(sevt->event_type, data, &time));
	fprintf(stderr, "received seq %lu\n", seq);
    } else {
	sevent_send_event(sevt, data, &time);
    }

  err_out:
    
    return 0;
}

int tag_err(struct spxml_cntn *cntn, struct xml_item *item )
{
    struct mastersoxml_object *this = (struct mastersoxml_object *)cntn->userdata;
    const char *seq_attr = xml_item_get_attr(item, "seq");
    fprintf(stderr, "xml cntn error: %s\n", item->text);

     if(seq_attr){
	unsigned long seq; 
	sscanf(seq_attr, "%lu", &seq);
	cntntxrx_rxseq(&this->txrx, seq, NULL);
		
    }

}

struct tag_handler tag_handlers[] = { 
    { "etype" ,  tag_etype } ,
    { "upd" , tag_upd },
    { "fin" , tag_fin },
    { "err" , tag_err },
    { NULL ,  NULL }   
};



int start_subscribe(XML_START_PAR)
{
    struct modules *modules = (struct modules*)data;
    struct module_base* this = ele->parent->data;
    struct event_handler* handler = NULL;

    /** @ToDo */

    return 0;
      
}




struct xml_tag module_tags[] = {                         
    { "listen", "module", start_subscribe, NULL, NULL},
    { "" , "" , NULL, NULL, NULL }
};
  

int socket_clt_rxtx(struct mastersoxml_object* this, const char *select)
{
    struct sevtread sevtrd;
    unsigned long seq = cntntxrx_txseq(&this->txrx, &sevtrd);
    struct xml_item *item = NULL;
    struct spxml_cntn *cntn = this->cntn;
    struct xml_doc* doc = spxml_cntn_tx_add_reserve(cntn);
    int retval = 0;

    if(!doc){
	return -EFAULT;
    }
    fprintf(stderr, "sending seq %lu\n", seq);
    item = xml_item_create(doc, "lst");
    if(!item){
	PRINT_ERR("failed to create xml item for list request");
	retval = -EFAULT;
	goto out;
    }
    
    xml_item_add_attr(doc,item, "select", xml_doc_text_dup(doc, select, 0));
    xml_item_add_attr(doc,item, "seq", xml_doc_text_printf(doc, "%lu", seq));
    
    if(!xml_item_add_child(doc, doc->first, item)){
	PRINT_ERR("could not add child");
	retval = -EFAULT;
	goto out;
    }

  out:

    spxml_cntn_tx_add_commit(cntn);

    if(retval == 0)
	cntntxrx_rxwait(&this->txrx, &sevtrd);
    else
	cntntxrx_cancel(&this->txrx, &sevtrd);	

    return retval;

}


 






int socket_clt_connect(struct sockaddr *skaddr){
    
    int len;
    int retval;
    int client_fd;
    int errors = 0;


    while(errors < 100){

	/* open socket */
	if((client_fd = socket(skaddr->sa_family, SOCK_STREAM, 0))<0){
	    fprintf(stderr, "server socket returned err: %s\n", strerror(errno));
	    errors++;
	    sleep(1);
	    continue;
	} 
	
	len = asocket_addr_len(skaddr);
	
	retval = connect(client_fd, skaddr, len);

	if(retval < 0){
	    fprintf(stderr, "connect error: %s (%d)\n",strerror(errno), errno);
	    fprintf(stderr, "closing socket\n");
	    close(client_fd);
	    errors++;
	    sleep(1);
	    continue;
	} else {
	    break;
	}

    }
    
    if(errors >= 100)
	return -1;



    if(skaddr->sa_family == AF_INET){
        int val;
        val = 10; /* 10 sec before starting probes */
        retval = setsockopt (client_fd, SOL_SOCKET, SO_KEEPALIVE , &val, sizeof (val));
        //   printf("SO_KEEPALIVE ret %d\n", retval);
        
        val = 10; /* 10 sec before starting probes */
        retval = setsockopt (client_fd, SOL_TCP, TCP_KEEPIDLE, &val, sizeof (val));
        // printf("TCP_KEEPIDLE ret %d\n", retval);
        
        val = 2; /* 2 probes max */
        retval = setsockopt (client_fd, SOL_TCP, TCP_KEEPCNT, &val, sizeof (val));
        // printf("TCP_KEEPCNT ret %d\n", retval);
        
        val = 10; /* 10 seconds between each probe */
        retval = setsockopt (client_fd, SOL_TCP, TCP_KEEPINTVL, &val, sizeof (val));
        // printf("TCP_KEEPINTVL ret %d\n", retval);
    }

    return client_fd;
    

}


int module_init(struct module_base *base, const char **attr)
{
    int client_fd = -1;
    struct mastersoxml_object* this = module_get_struct(base);
    struct sockaddr *skaddr;
    const char *ip = get_attr_str_ptr(attr, "ip");
    int port = get_attr_int(attr, "port",  DEFAULT_PORT);
    
    cntntxrx_init(&this->txrx);

    if(!ip){
	PRINT_ERR("No ip address");
	return -1;
    }

    if(port < 0 || port > 0xffff){
	PRINT_ERR("Invalid port: %d", port);
	return -1;
    }	

    skaddr = asocket_addr_create_in(ip,port);
    if(!skaddr){
	PRINT_ERR("Invalid address");
	return -1;
    }

    //Open socket
    client_fd = socket_clt_connect(skaddr);
     if(client_fd<0){
	PRINT_ERR("Could not connect to slave");
	return -1;
     }
    
    // Create cntn

     this->cntn = spxml_cntn_create(client_fd, tag_handlers, (void*)this, base->verbose);

     if(this->cntn==NULL){
	 PRINT_ERR("Could not connect to slave module");
	 return -1;
     }
     
     socket_clt_rxtx(this, "*.*");

     return 0;
}


void module_deinit(struct module_base *base)
{
    struct mastersoxml_object* this = module_get_struct(base);

    spxml_cntn_delete(this->cntn);
    cntntxrx_deinit(&this->txrx);
}





struct module_type module_type = {            
    .name       = "xsocketmaster",                   
    .xml_tags   = module_tags ,                      
    .type_struct_size = sizeof(struct mastersoxml_object), 
};


struct module_type *module_get_type()
{
    return &module_type;
}
