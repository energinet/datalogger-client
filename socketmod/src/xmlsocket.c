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
#include "xml_cntn.h"
#include "xmlcontdaem.h"
#include <xml_cntn_server.h>

#define DEFAULT_PORT 6527
#define DEFAULT_INSCNT 10



struct soxml_object{
    struct module_base base;
    struct xml_cntn_server *server;
    int echo_write;
};



struct soxml_object* module_get_struct(struct module_base *module){
    return (struct soxml_object*) module;
}

int spxml_cntn_tx_add_upd(struct xml_cntn_server *server, struct spxml_cntn *cntn, struct module_event *event, const char *seq)
{
    int retval = 0;
    struct xml_item *item = NULL;
    struct xml_doc* doc = NULL;
    int dbglev = (cntn)?cntn->dbglev:server->dbglev;
    doc =  xml_doc_create(10*1024, dbglev);
    
    if(!doc){
	return -EFAULT;
    } 

    item = module_event_xml_upd(event, doc);
    if(!item){
	PRINT_ERR("failed to create xml item for event");
	retval = -EFAULT;
	goto out;
    }

    if(cntn)
	spxml_cntn_send(cntn, item, seq);
    else if(server)
	xml_cntn_server_broadcast(server, item);

    
  out:
    xml_doc_delete(doc);

    return retval;
}




int tag_list(struct spxml_cntn *cntn, struct xml_item *rxitem )
{

    struct soxml_object *this = (struct soxml_object *)cntn->userdata;
    struct event_search search;
    struct event_type *etype;
    struct xml_doc* doc = xml_doc_create(100*1024, this->base.verbose);
    //spxml_cntn_tx_add_reserve(cntn);
    const char *seq_attr = xml_item_get_attr(rxitem, "seq");
    const char *select_attr = xml_item_get_attr(rxitem, "select");
    int retval = 0;
    struct xml_item *txitem ;

    if(!doc){
	PRINT_ERR("could not create doc");
	return -EFAULT;
    }

    txitem = xml_item_create(doc, "etypes");

    event_search_init(&search, this->base.first, select_attr, FLAG_ALL);

    while(etype = event_search_next(&search)){
		struct xml_item *item = event_type_xml(etype, doc);
		if(!xml_item_add_child(doc, txitem, item)){
			PRINT_ERR("could not add item");
			break;
		}
    }

    spxml_cntn_send(cntn, txitem, seq_attr);
      
  out:
    xml_doc_delete(doc);
    
    return retval;
}
 
int tag_read(struct spxml_cntn *cntn, struct xml_item *item )
{
    
    char errbuf[256];
    struct soxml_object *this = (struct soxml_object *)cntn->userdata;
    struct module_base* base = &this->base;
    const char *etype_attr = xml_item_get_attr(item, "ename");
    const char *seq_attr = xml_item_get_attr(item, "seq");

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

    if(spxml_cntn_tx_add_upd(NULL, cntn, event, seq_attr)<0)
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

    etype = module_base_get_event(base->first, etype_attr, FLAG_ALL );
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
	    if(spxml_cntn_tx_add_upd(NULL, cntn, event,seq_attr )<0)
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
    
int module_init(struct module_base *base, const char **attr)
{
    struct soxml_object* this = module_get_struct(base);
    
    int inscnt =  get_attr_int(attr, "instances",  DEFAULT_INSCNT);
    int port = get_attr_int(attr, "port", DEFAULT_PORT); ;

    this->echo_write = get_attr_int(attr, "echo_write", 1); ;  
    
    if(port < 0 || port > 0xffff){
	PRINT_ERR("Invalid port: %d", port);
	return -1;
    }	

    if(inscnt<=0){
	PRINT_ERR("Invalid instance count: %d", inscnt);
	return -1;
    }
    
    this->server = xml_cntn_server_create((void*)this, port, inscnt, tag_handlers, base->verbose);

    if(!this->server){
	PRINT_ERR("Could not create server object");
	return -1;
    }
	
    if(xml_cntn_server_start(this->server)<0){
	PRINT_ERR("Could not start server object");
	goto err_out;
    }

    return 0;
    
 err_out:
    xml_cntn_server_delete(this->server);
    return -1;

}


void module_deinit(struct module_base *module)
{
    struct soxml_object* this = module_get_struct(module);

    xml_cntn_server_stop(this->server);
    modemd_server_delete(this->server);

    return;
}



int event_handler_all(EVENT_HANDLER_PAR)
{
    struct soxml_object* this = module_get_struct(handler->base);
    struct spxml_cntn *cntn = ( struct spxml_cntn*)handler->objdata;
    struct spxml_cntn *ptr = NULL;
    int retval = 0;
    struct timespec abs_time;

    if(spxml_cntn_tx_add_upd(this->server, NULL, event, NULL)<0)
	PRINT_DBG(handler->base->verbose >=2, "Could not add event"); ;
    
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
