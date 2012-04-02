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
#include "xmlcntn.h"
#include "xmlcontdaem.h"

#define DEFAULT_PORT 6527


struct soxml_object{
    struct module_base base;
    struct socket_data sparm;
    struct spxml_cntn *cntns;
    pthread_mutex_t cntn_mutex;
    pthread_t s_thread;  
    int echo_write;
    int port;
};



struct soxml_object* module_get_struct(struct module_base *module){
    return (struct soxml_object*) module;
}




int spxml_cntn_tx_add_upd(struct spxml_cntn *cntn, struct module_event *event, const char *seq)
{
    int retval = 0;
    struct xml_item *item = NULL;
    struct xml_doc* doc = NULL;

    if(!cntn)
	return -EFAULT;
    
    if(cntn->run==0)
	return -EFAULT;

    doc = spxml_cntn_tx_add_reserve(cntn);
    
    if(!doc){
	return -EFAULT;
    } 

    item = module_event_xml_upd(event, doc);
    if(!item){
	PRINT_ERR("failed to create xml item for event");
	retval = -EFAULT;
	goto out;
    }

    if(seq){
	xml_item_add_attr(doc, item, "seq", xml_doc_text_dup(doc, seq,0));
	fprintf(stderr, "sending seq %s\n", seq);
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




int tag_list(struct spxml_cntn *cntn, struct xml_item *item )
{

    struct soxml_object *this = (struct soxml_object *)cntn->userdata;
    struct event_search search;
    struct event_type *etype;
    struct xml_doc* doc = spxml_cntn_tx_add_reserve(cntn);
    const char *seq_attr = xml_item_get_attr(item, "seq");
    const char *select_attr = xml_item_get_attr(item, "select");
    int retval = 0;

    if(!doc){
	PRINT_ERR("could not get doc");
	return -EFAULT;
    }

    event_search_init(&search, this->base.first, select_attr, FLAG_ALL);

    while(etype = event_search_next(&search)){
	struct xml_item *item = event_type_xml(etype, doc);
	if(!xml_item_add_child(doc, doc->first, item)){
	    PRINT_ERR("could not add item");
	    break;
	}
    }

    if(seq_attr){
	struct xml_item *item = xml_item_create(doc, "fin");
	xml_item_add_attr(doc, item, "seq", xml_doc_text_dup(doc, seq_attr,0));

	if(!xml_item_add_child(doc, doc->first, item)){
	    PRINT_ERR("could not add item");
	}
    }

  out:
    spxml_cntn_tx_add_commit(cntn);
    
    return retval;
}
 
int tag_read(struct spxml_cntn *cntn, struct xml_item *item )
{
    
    char errbuf[256];
    struct soxml_object *this = (struct soxml_object *)cntn->userdata;
    struct module_base* base = &this->base;
    const char *etype_attr = xml_item_get_attr(item, "ename");
    const char *seq_attr = xml_item_get_attr(item, "seq");

    fprintf(stderr, "read %s %s\n", etype_attr, seq_attr );
    
    if(!etype_attr){
	snprintf(errbuf, sizeof(errbuf), "no etype attribut in tag %s", item->name);
	goto err_out;
    }

    struct event_type *etype =  module_base_get_event(base->first, etype_attr, FLAG_ALL );
    if(!etype){
	snprintf(errbuf, sizeof(errbuf), "unknown etype %s", etype_attr);
	goto err_out;
    }

    struct module_event *event = event_type_read(etype);

    if(!event){
	snprintf(errbuf, sizeof(errbuf), "read not supported for etype %s", etype_attr);
	goto err_out;
    }

    if(spxml_cntn_tx_add_upd(cntn, event, seq_attr)<0)
	PRINT_ERR("Could not add event");  
    module_event_delete(event);

    return 0;

  err_out:
    
    spxml_cntn_tx_add_err(cntn, errbuf,seq_attr );
    return 0;

}

int tag_write(struct spxml_cntn *cntn, struct xml_item *item )
{
    char errbuf[256];
    int retval = 0;
    struct soxml_object *this = (struct soxml_object *)cntn->userdata;
    struct module_base* base = &this->base;
    const char *etype_attr = xml_item_get_attr(item, "ename");
    const char *seq_attr = xml_item_get_attr(item, "seq");
    struct event_type *etype = NULL; 
    if(!etype_attr){
	snprintf(errbuf, sizeof(errbuf), "no etype attribut in tag %s", item->name);
	goto err_out;
    }

    struct xml_item *data_item = xml_item_get_child(item, NULL, "data");
    if(!data_item){
	snprintf(errbuf, sizeof(errbuf), "no data etype %s", etype_attr);
	goto err_out;
    }

    struct uni_data *data = uni_data_create_from_xmli(data_item);
    if(!data_item){
	snprintf(errbuf, sizeof(errbuf), "error reading data from %s", etype_attr);
	goto err_out;
    }

    etype = module_base_get_event(base->first, etype_attr, FLAG_SHOW );
    if(!etype){
	snprintf(errbuf, sizeof(errbuf), "unknown etype %s", etype_attr);
	goto err_out;
    }
    
    retval = event_type_write(etype, data);
    if(retval<0){
	snprintf(errbuf, sizeof(errbuf), "error writing %s: %s", etype_attr, strerror(-retval));
	goto err_out;
    }

    if(this->echo_write){
	struct module_event *event = event_type_read(etype);
	if(event){
	    if(spxml_cntn_tx_add_upd(cntn, event,seq_attr )<0)
		PRINT_ERR("Could not add event"); 


	     module_event_delete(event);
	}
    }

    return 0;

  err_out:
    
    spxml_cntn_tx_add_err(cntn, errbuf,seq_attr );
    return 0;
}

int tag_err(struct spxml_cntn *cntn, struct xml_item *item )
{
    fprintf(stderr, "xml cntn error: %s\n", item->text);
}


struct tag_handler tag_handlers[] = { 
    { "lst" ,  tag_list } ,
    { "rd" ,  tag_read } ,
    { "wrt" , tag_write },
    { "err" , tag_err },
    { NULL ,  NULL }   
};



int start_listen(XML_START_PAR)
{
    struct modules *modules = (struct modules*)data;
    struct module_base* this = ele->parent->data;
    struct event_handler* handler = NULL;

    if(!this)
	return 0;

    handler = event_handler_get(this->event_handlers, "upd");

    assert(handler); 

    return module_base_subscribe_event(this, modules->list, get_attr_str_ptr(attr, "event"),FLAG_ALL,  handler, attr);
}


struct xml_tag module_tags[] = {                         
    { "listen", "module", start_listen, NULL, NULL},
    { "" , "" , NULL, NULL, NULL }
};
    



void *xsckt_srv_loop(void *param) 
{ 
    struct socket_data *this = (struct socket_data*)param;
    struct soxml_object* mod = (struct soxml_object*)this->appdata;
    struct sockaddr_in  remote; //Todo
    int client_fd;
   
    int retval ;
    int optval = 1;          /* setsockopt */
    socklen_t t;
    int len;
    int errors = 0;

    while(errors < 100){

    /* open socket */
    if((this->socket_fd = socket(this->skaddr->sa_family, SOCK_STREAM, 0))<0){
        PRINT_ERR("server socket returned err: %s", strerror(errno));
	errors++;
	sleep(1);
        continue;
    }
     
    if (setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0){
        PRINT_ERR( "server setsockopt returned err: %s", strerror(errno));
        retval = -errno;
        goto out;
    }

    len =  asocket_addr_len(this->skaddr);

    if(bind(this->socket_fd, this->skaddr, len)<0){
        PRINT_ERR("server bind returned err: %s", strerror(errno));
        retval = -errno;
        goto out;
    }
    
    if(listen(this->socket_fd, 1024)<0){
        PRINT_ERR("server listen returned err: %s", strerror(errno));
        retval = -errno;
        goto out;
    }

    PRINT_DBG(this->dbglev, "Socket opened...");

    while(this->run){
        struct asocket_con* sk_con;
        t = sizeof(remote);

        if((client_fd = accept(this->socket_fd, (struct sockaddr *)&remote, &t))<0){
	    PRINT_ERR( "accept returned err: %s", strerror(errno));
            retval = -errno;
            goto out;
        }
     
	PRINT_DBG(this->dbglev, "Socket: client connected...");

	if(pthread_mutex_lock(&mod->cntn_mutex)!=0)
	    goto out;
	mod->cntns = spxml_cntn_add(mod->cntns, spxml_cntn_create(client_fd, tag_handlers, (void*)mod, this->dbglev));

	PRINT_DBG(this->dbglev, "Socket connection cleaning");
	mod->cntns = spxml_cntn_clean(mod->cntns);
	PRINT_DBG(this->dbglev, "Socket connection cleaned");
	pthread_mutex_unlock(&mod->cntn_mutex);
	
	PRINT_DBG(this->dbglev, "Socket: Connection closed");

    }
    }
  out:
    close(this->socket_fd);
    this->socket_fd = -1;
    return NULL;

}



struct sockaddr *asocket_addr_create_in(const char *ip, int port);




int module_server_start( struct soxml_object* this)
{
    
    int retval = 0;
    struct socket_data *param =  &this->sparm;
    fprintf(stderr, "starting first stread\n");

    param->appdata = (void*)this;
    param->skaddr = asocket_addr_create_in(NULL,  this->port);
    param->cmds = NULL;
    param->dbglev = this->base.verbose;
    param->run = 1;
    if((retval = pthread_create(&this->s_thread, NULL, xsckt_srv_loop, param))<0){  
	PRINT_ERR("Failure starting loop in %p: %s ", this, strerror(retval));  
	return retval;
    }      

    return 0;

}

int module_init(struct module_base *base, const char **attr)
{
    struct soxml_object* this = module_get_struct(base);
    

    this->echo_write = get_attr_int(attr, "echo_write", 1); ;
    
    pthread_mutex_init(&this->cntn_mutex, NULL);

    this->port = get_attr_int(attr, "port", DEFAULT_PORT); ;

    module_server_start(this);

    return 0;

}


void module_deinit(struct module_base *module)
{
    struct soxml_object* this = module_get_struct(module);

    //asckt_srv_close(&this->param);
    
    pthread_mutex_destroy(&this->cntn_mutex);
    
    spxml_cntn_delete(this->cntns);


    return;
}



int event_handler_all(EVENT_HANDLER_PAR)
{
    struct soxml_object* this = module_get_struct(handler->base);
    struct spxml_cntn *cntn = ( struct spxml_cntn*)handler->objdata;
    struct spxml_cntn *ptr = NULL;
    int retval = 0;

//    PRINT_MVB(handler->base, "receive event %s.%s \n", event->source->name, event->type->name);

    if((retval = pthread_mutex_lock(&this->cntn_mutex))!=0){
	if(retval != EBUSY)
	    return -EFAULT;
	fprintf(stderr, "this->cntn_mutex is locked");
	if(pthread_mutex_lock(&this->cntn_mutex)!=0)
	    return -EFAULT;
    }

    ptr = this->cntns;
    while(ptr){
	if(spxml_cntn_tx_add_upd(ptr, event, NULL)<0)
	    PRINT_DBG(handler->base->verbose >=2, "Could not add event"); ;
	ptr = ptr->next;
    }
    pthread_mutex_unlock(&this->cntn_mutex);
//    PRINT_MVB(handler->base, "added event %s.%s \n", event->source->name, event->type->name);
    //  spxml_cntn_tx(this->cntns);
    
//    PRINT_MVB(handler->base, "send event %s.%s \n", event->source->name, event->type->name);
    
    return 0;

}


struct event_handler handlers[] = { {.name = "upd", .function = event_handler_all} ,{.name = ""}};

struct event_handler *module_subscribe(struct module_base *module, struct module_base *source, struct event_type *event_type,
                     const char **attr)
{
    struct soxml_object *this = module_get_struct(module);
    struct event_handler* handler = NULL;
    char *type;
    char ename[64];
    if(strcmp(event_type->name, "fault")==0)
	return NULL;

    sprintf(ename, "%s.%s", source->name, event_type->name);
    PRINT_MVB(module, "subscribe %s", ename);
    
    handler = event_handler_get(this->base.event_handlers, "upd");
    printf("-->%p %s\n", handler, type);
    return handler;


}



struct module_type module_type = {            
    .name       = "xsocket",                   
    .xml_tags   = module_tags ,                      
    .handlers   = handlers ,                        
    .type_struct_size = sizeof(struct soxml_object), 
};


struct module_type *module_get_type()
{
    return &module_type;
}
