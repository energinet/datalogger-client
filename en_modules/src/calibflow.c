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
#include <linux/input.h>
#include <assert.h>

#define MAXMODES 10


enum calib_states{
    CALIB_STOPPED,
    CALIB_STOPPEDWFLOW,
    CALIB_AWAITFLOW,
    CALIB_RUNNING,
    CALIB_FINISHED,
    CALIB_SEND,
    CALIB_ERROR,
};


struct module_object{
    struct module_base base;
    enum calib_states state;
    float flow;
    float count;
    float threshold_high;
    float threshold_low;
    struct timeval t_start;
    struct timeval t_end;
    struct event_type *status_event;
};


struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}




int calibflow_status_print(struct module_object* this, char*buffer, size_t size)
{
    int len= 0;
    time_t duration = 0;
    char *states[] = { "Stoppet", "Vand løber", "Klar", "Måler", "Færdig", "Sendt", "Fejl" };

    len += snprintf(buffer+len, size-len, "%s", states[this->state]);

    switch (this->state){
      case CALIB_ERROR:
      case CALIB_AWAITFLOW:
	len += snprintf(buffer+len, size-len, ": %2.2f l/min", this->flow);
	break;
      case CALIB_STOPPEDWFLOW:
      case CALIB_RUNNING:
	duration = 1 + time(NULL) - this->t_start.tv_sec;
	len += snprintf(buffer+len, size-len, ": %2.2f l  / %d sek =  %2.2f l/min", this->count, duration, 
			(this->count/duration)*60);
	break;
      case CALIB_STOPPED:
      case CALIB_FINISHED:
      case CALIB_SEND:
	duration = this->t_end.tv_sec - this->t_start.tv_sec;
	len += snprintf(buffer+len, size-len, ": %2.2f l  / %d sek =  %2.2f l/min", this->count, duration, 
			(this->count/duration)*60);
	break;
    }

    return len;
}


void calibflow_status_print_dbg(struct module_object* this)
{
    char buffer[256];
    calibflow_status_print(this, buffer, 256);
    fprintf(stderr, "->>%s\n", buffer);
    
}

struct uni_data *calibflow_status_data(struct module_object* this)
{
    
    char buffer[256];
    calibflow_status_print(this, buffer, 256);
    return uni_data_create_state(this->state, buffer);
}

struct module_event *calibflow_eread(struct event_type *event)
 {
     struct module_object* this = module_get_struct(event->base);
     
     return module_event_create(event, calibflow_status_data(this), NULL);
 }


void calibflow_set_starting( struct module_object* this)
{

    PRINT_MVB(&this->base, "set starting %f %f",this->flow , this->threshold_low );

    if(this->flow > this->threshold_low){
	 this->state = CALIB_ERROR;
	 return;
    }


    this->state = CALIB_AWAITFLOW; 

	
    return;
}
    

void calibflow_set_stopped(struct module_object* this)
{
    this->state = CALIB_STOPPED;
    PRINT_MVB(&this->base, "stopped");
    this->count = 0;
    memcpy(&this->t_start, &this->t_end, sizeof(this->t_start));

    
}

void calibflow_set_run(struct module_object* this)
{
    if(this->state == CALIB_STOPPED){
	this->state = CALIB_STOPPEDWFLOW;
    } else {
	this->state = CALIB_RUNNING;
    }
    
    this->count = this->flow/60;
//    this->count = 0;
    gettimeofday(&this->t_start, NULL);

    PRINT_MVB(&this->base, "now running");

    return;

}

void calibflow_set_finished(struct module_object* this)
{
    if(this->state == CALIB_RUNNING){
	this->state = CALIB_FINISHED;
    } else {
	this->state = CALIB_STOPPED;
    }

    gettimeofday(&this->t_end, NULL);

    PRINT_MVB(&this->base, "finished");
}

void calibflow_send(struct module_object* this)
{
    module_base_send_event(module_event_create(this->status_event, calibflow_status_data(this), NULL)); 
    this->state = CALIB_SEND;
    PRINT_MVB(&this->base, "send");
}



int handler_flow(EVENT_HANDLER_PAR)
{
    struct module_object* this = module_get_struct(handler->base);
    struct uni_data *data = event->data;
    float flow = uni_data_get_fvalue(event->data); 
    PRINT_MVB(&this->base, "state %d %f %d.%6.6d",this->state , this->flow , event->time.tv_sec,  event->time.tv_usec );
    switch (this->state){
      case CALIB_STOPPED:
      case CALIB_AWAITFLOW:
	this->flow =  flow;
	PRINT_MVB(&this->base, "AWAITFLOW %f %f",this->flow , this->threshold_high );
	if(this->flow > this->threshold_high)
	    calibflow_set_run(this);
	break;
      case CALIB_STOPPEDWFLOW:
      case CALIB_RUNNING:
	PRINT_MVB(&this->base, "RUNNING %f %f",this->flow , this->threshold_high );
	this->flow =  flow;
	this->count += this->flow/60;
	if(this->flow < this->threshold_low)
	    calibflow_set_finished(this);
	break;
      case CALIB_FINISHED:
      case CALIB_SEND:
      case CALIB_ERROR:
	break;
    }
    

    return 0;
}

int handler_cmd(EVENT_HANDLER_PAR)
{
    struct module_object* this = module_get_struct(handler->base);
    struct uni_data *data = event->data;
    int pressed = data->value;
    int sec     = data->mtime/1000;
    //calibflow_status_print_dbg(this);
    PRINT_MVB(&this->base, "cmd received %d %f", data->value, (float)data->mtime/1000);
    //calibflow_status_print_dbg(this);

    if((pressed == 0) && (sec > 3)){
	if(this->state == CALIB_FINISHED){
	    calibflow_send(this);
	} else {
	    calibflow_set_starting(this);	    
	}
    } else if (pressed == 0){
	calibflow_set_stopped(this);
    }

    

    


    return 0;
}


int start_input(XML_START_PAR) 
{
    int retval;
    struct modules *modules = (struct modules*)data; 
    struct module_base* base = ele->parent->data; 
    struct module_object* this = module_get_struct(base);
    struct event_handler *flow_event_handler = NULL;
    struct event_handler *cmd_event_handler = NULL;
    const char *event_flow = get_attr_str_ptr(attr, "flow");
    const char *event_cmd = get_attr_str_ptr(attr, "cmd");

    if(!this) 
 	return 0; 

    PRINT_MVB(base, "event_flow: %s event_cmd: %s", event_flow , event_cmd);

       /* create an event handler for receiving commands */
    cmd_event_handler  = event_handler_create("cmdrcv",  handler_cmd  ,base, NULL);
    base->event_handlers = event_handler_list_add(base->event_handlers, flow_event_handler);

    /* create an event handler for receiving flow input*/
    flow_event_handler = event_handler_create("flowrcv", handler_flow ,base, NULL);  
    base->event_handlers = event_handler_list_add(base->event_handlers, cmd_event_handler);

    /* Create an event type for status notifications */
    this->status_event = event_type_create(base, NULL, "status" , ".", "Flowkalibrering" ,  
		      event_type_get_flags(get_attr_str_ptr(attr, "flags"), base->flags)); 

    /* Add a eread handler */
    this->status_event->read = calibflow_eread;
    
    base->event_types = event_type_add(base->event_types, this->status_event); // make it avaliable

    PRINT_MVB(base, "->read: %p eread %p", this->status_event->read, calibflow_eread);
    
      if((retval = module_base_subscribe_event(base, base->first, event_cmd, FLAG_ALL, 
					     cmd_event_handler, attr))<0){
	  PRINT_ERR("Calibflow could not subscribe to %s", event_cmd);
	return retval;
    }

    if((retval = module_base_subscribe_event(base, base->first, event_flow, FLAG_ALL, 
					     flow_event_handler, attr))<0){
	PRINT_ERR("Calibflow could not subscribe to %s", event_flow);
	return retval;
    }
    
} 

struct xml_tag module_tags[] = {					\
    { "inputs" , "module" , start_input, NULL, NULL},			\
    { "" , "" , NULL, NULL, NULL }                       \
};  



int module_init(struct module_base *base, const char **attr)
{
    int retval = 0;
    struct module_object* this = module_get_struct(base);

    this->threshold_high = 2;
    this->threshold_low = 1;
    this->state = CALIB_STOPPED;
    return 0;
}



struct event_handler handlers[] = { {.name = "rcv", .function = handler_flow} ,{.name = ""}};


struct module_type module_type = {                  \
    .name       = "calibflow",                          \
    .mtype      = MTYPE_CONTROL,                    \
    .subtype    = SUBTYPE_EVENT,                     \
    .xml_tags   = module_tags,		     \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct module_object), \
};


struct module_type *module_get_type()
{
    return &module_type;
}
