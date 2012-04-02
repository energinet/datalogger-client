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
#include <sys/statvfs.h> 


#include <asocket.h>
#include <asocket_client.h>


#define DEFAULT_INTERVAL (2)  /* sec */
#define DEFAULT_UNIT ""
#define DEFAULT_PORT 7414


struct module_object{
    struct module_base base;
    int interval;
    struct event_type *event;
    struct event_type *state_event;
    int state;
    struct sys_info *sysitems;
};



void licon_send_m_event(struct module_object *module, struct event_type *event_type, struct timeval *time, unsigned long diff, char *text)
{
    /* struct uni_data data; */

    /* memset(&data, 0, sizeof(data)); */

    /* data.type = DATA_TEXT; */
    /* data.value = 0; */
    /* data.mtime = diff; */
    /* data.data = text; */

    //memcpy(&data.time, time, sizeof(data.time));

//    module->base.send_event(&module->base, event_type, &data);
//    module_base_send_event(&module->base, event_type, &data);
    module_base_send_event(module_event_create(event_type, uni_data_create_text(text), time));
}


int licon_log_get_state(void) 
{ 
    struct asocket_con* sk_con = NULL;
    struct skcmd* tx_msg = NULL;
    struct skcmd* rx_msg = NULL;
    struct sockaddr * skaddr = asocket_addr_create_in( "127.0.0.1", DEFAULT_PORT);
    int retval = -1;
   

    sk_con = asocket_clt_connect(skaddr);
    
    free(skaddr);
    
    if(!sk_con){
        fprintf(stderr, "Socket connect failed\n");
        return -errno;
    }

    tx_msg = asocket_cmd_create("StateGet");
    rx_msg = NULL;//asocket_cmd_create("State");
    
    asocket_con_trancive(sk_con, tx_msg, &rx_msg);

    if(rx_msg){
	if(rx_msg->param){
	    const char *state = asocket_cmd_param_get(rx_msg, 0);
	    if(strcmp(state, "down")==0)
		retval = 0;
	    else if(strcmp(state, "netup")==0)
		retval = 1;
	    else if(strcmp(state, "appup")==0)
		retval = 2;
	}
	asocket_cmd_delete(rx_msg);
    }
    
    asocket_cmd_delete(tx_msg);

    asocket_clt_disconnect(sk_con);  

    return retval;
} 

int licon_log_upd_state(struct module_object *module)
{
    int new_state = licon_log_get_state();

    if(new_state != module->state){
	struct uni_data data;
	struct timeval time;
	PRINT_MVB(&module->base, "Fault state changed %d -- %d", module->state,  new_state);
	gettimeofday(&time, NULL);

	memset(&data, 0, sizeof(data));

	data.type = DATA_INT;
	data.value = new_state ;
	data.mtime = 0;
	
//	memcpy(&data.time, &time, sizeof(data.time));
	
//	module_base_send_event(&module->base, module->state_event, &data);
	module_base_send_event(module_event_create(module->state_event, &data, &time));
	module->state = new_state;
    }

    return new_state;

}


int licon_log_read(struct module_object *module)
{

    struct asocket_con* sk_con = NULL;
    struct skcmd* tx_msg = NULL;
    struct skcmd* rx_msg = NULL;
    struct timeval time;
    struct sockaddr * skaddr = asocket_addr_create_in( "127.0.0.1", DEFAULT_PORT);
    int retval = 0;

    sk_con = asocket_clt_connect(skaddr);
    
       
    free(skaddr);
    
    if(!sk_con){
        fprintf(stderr, "Socket connect failed\n");
        return -errno;
    } else {
	sk_con->dbglev = module->base.verbose;
    }

    tx_msg = asocket_cmd_create("LogGet");
    rx_msg = NULL;//asocket_cmd_create("Log");

    asocket_con_trancive(sk_con, tx_msg, &rx_msg);
    
    if(rx_msg){
        if(rx_msg->param){
        
            PRINT_MVB(&module->base,"received event %s\n", asocket_cmd_param_get(rx_msg, 1));
            
            sscanf(asocket_cmd_param_get(rx_msg, 0), "%ld", &time.tv_sec);
            time.tv_usec = 0;
	    
	    PRINT_MVB(&module->base,"time is %ld (%s)\n", time.tv_sec,asocket_cmd_param_get(rx_msg, 0) );
            
            licon_send_m_event(module, module->event, &time, 1, asocket_cmd_param_get(rx_msg, 1));
            retval = 1;
        }
        asocket_cmd_delete(rx_msg);
    }
    
    asocket_cmd_delete(tx_msg);

    asocket_clt_disconnect(sk_con);  

    return retval;

}


struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}

int event_handler_all(EVENT_HANDLER_PAR)
{
    struct module_object* this = module_get_struct(handler->base);
    printf("Module type %s received event\n", this->base.name);
    return 0;
}

struct event_handler handlers[] = { {.name = "all", .function = event_handler_all} ,{.name = ""}};






struct xml_tag module_tags[] = {                         \
    { "" , "" , NULL, NULL, NULL }                       \
};
    
int module_init(struct module_base *module, const char **attr)
{
    struct module_object* this = module_get_struct(module);
    
    this->event = event_type_create(&this->base, NULL, "status", "", "Network",  DEFAULT_FFEVT); // create 

    if(! this->event){
        PRINT_ERR("event is NULL");
        return -1;
    }    

    this->state_event = event_type_create(&this->base, NULL, "state", "",
					  "Network State",  DEFAULT_FFEVT); // create 

    if(! this->state_event){
        PRINT_ERR("event is NULL");
        return -1;
    }    


    this->interval = get_attr_int(attr, "interval" , DEFAULT_INTERVAL);
    PRINT_MVB(module, "licon read interval: %d sec", this->interval);
    
    module->event_types = event_type_add(module->event_types,this->event); // make it avaliable
    module->event_types = event_type_add(module->event_types,this->state_event); // make it avaliable


    this->state = -1;

    return 0;
}


void* module_loop(void *parm)
{
    struct module_object *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;
    int retval;
    time_t prev_time;
    base->run = 1;
    
    while(base->run){ 
	licon_log_upd_state(this);
        while(licon_log_read(this)==1);
        sleep( this->interval);
    } 
    
    PRINT_MVB(base, "loop returned");
    
    return NULL;

}

struct module_type module_type = {                  \
    .name       = "licon",                       \
    .mtype      = MTYPE_INPUT,                    \
    .subtype    = SUBTYPE_LEVEL,                     \
    .xml_tags   = module_tags,                      \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct module_object), \
};


struct module_type *module_get_type()
{
    return &module_type;
}
