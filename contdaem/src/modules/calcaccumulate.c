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


/**
 * @ingroup modules 
 * @{
 */

/**
 * @defgroup modules_calcacc accumulate module
 * @{
 * Module for averaging or summing events for a given interval.
 * <b>Typename: "accumulate" </b>\n
 * <b>Attributes:</b>\n
 * <ul>
 * <li> \b interval: The interval to sum or average. Default is @ref DEFAULT_ACCINTERVAL
 * </ul>
 * <b>Tags:</b>
 * <ul>
 * <li><b>input:</b> Configure input device\n  
 *  This tag supports mass subscribe see the @ref conf_subs_src section at the main page for details.
 * - \b interval: The interval to sum or average. 
 *      Default is the interval attribute specified in the module tag.
 * - \b output: The output data type. Possible are \p level 
 *      or as the \p input event datatype. Default is \p input.
 * </ul>
 */


#define DEFAULT_ACCINTERVAL 300

enum acc_output{ 
    ACCOUT_INPUT, /*!< Output as input */
    ACCOUT_LEVEL, /*!< Output is avage level */
    ACCOUT_FLOW,  /*!< (not implemented)Outout is flow */
    ACCOUT_SUM,   /*!< Output sum  */
    ACCOUT_MIN,   /*!< Output mimumum */
    ACCOUT_MAX,   /*!< Output maximum */
}; 

const char *outtyps[] = { "input", "level", "flow" , "sum","min","max", NULL};



struct acc_obj{
    enum data_types dtype;
    enum acc_output output;
    int count;
    float value;
    int interval;
    struct timeval t_send;
    struct event_type *event_type;
    struct acc_obj *next;
};


struct module_object{
    struct module_base base;
    int interval;
    struct acc_obj *accobjs;
};

struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}


struct acc_obj *acc_obj_create(const char **attr)
{
    const char *output = get_attr_str_ptr(attr, "output");
    struct acc_obj *new = malloc(sizeof(*new));
    assert(new);
    memset(new, 0, sizeof(*new));
    
    new->output = modutil_get_listitem(output, ACCOUT_INPUT, outtyps);

    return new;
}

struct acc_obj* acc_obj_add(struct acc_obj *list,struct acc_obj *new){
    struct acc_obj *ptr = list;
    
    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}


void acc_obj_delete(struct acc_obj *acc_obj)
{
    if(!acc_obj)
	return ;

    acc_obj_delete(acc_obj->next);
    free(acc_obj);
}



int handler_def(EVENT_HANDLER_PAR)
{
    return 0;
}


int handler_rcv(EVENT_HANDLER_PAR)
{
    struct module_object* this = module_get_struct(handler->base);
    struct acc_obj *accobj = (struct acc_obj*)handler->objdata;
    struct uni_data *data = event->data;
    float fvalue =  uni_data_get_fvalue(data);

    PRINT_MVB(handler->base, "received event from %s.%s count %d------------>>>>\n", event->source->name, event->type->name, accobj->count);

    accobj->dtype = data->type;
    
    switch(accobj->output){
      case ACCOUT_MIN:
	if(( accobj->count == 0)||(accobj->value > fvalue))
	    accobj->value = fvalue; 
	break;
      case ACCOUT_MAX:
	if(( accobj->count == 0)||(accobj->value < fvalue))
	    accobj->value = fvalue; 
	break;
      case ACCOUT_INPUT:
      case ACCOUT_LEVEL:
      case ACCOUT_FLOW:
      case ACCOUT_SUM:
      default:
	accobj->value +=  uni_data_get_value(data);
	break;
    }
	
    accobj->count++;
    // printf("Acc %s value %f, acc %f avg %f %d\n", accobj->event_type->name, uni_data_get_fvalue(data),  accobj->value, accobj->value/accobj->count, accobj->dtype);

    return 0;
}



struct event_handler handlers[] = { {.name = "def", .function = handler_def}, \
				    {.name = "rcv", .function = handler_rcv} ,{.name = ""}};



int start_avg(XML_START_PAR)
{
    struct modules *modules = (struct modules*)data;
    struct module_base* this = ele->parent->data;
    struct event_handler* handler = NULL;

    if(!this)
	return 0;

    handler = event_handler_get(this->event_handlers, "def");

    assert(handler); 

    return module_base_subscribe_event(this, modules->list, get_attr_str_ptr(attr, "event"), FLAG_ALL,  handler, attr);


}

struct xml_tag module_tags[] = {                         \
    { "acc"  , "module" , start_avg, NULL, NULL},   \
    { "avg"  , "module" , start_avg, NULL, NULL},	  \
    { "" , ""  , NULL, NULL, NULL }                       \
};
    
int module_init(struct module_base *module, const char **attr)
{
    struct module_object* this = module_get_struct(module);

    this->interval = get_attr_int(attr, "interval", DEFAULT_ACCINTERVAL);

}



int acc_obj_send(struct module_object *module, struct acc_obj *accobj, struct timeval *time)
{
    if(accobj->count || accobj->dtype == DATA_FLOW){
	struct uni_data *data;
	
	if(accobj->dtype == DATA_FLOW){
	    if(accobj->output == ACCOUT_SUM||accobj->output == ACCOUT_MIN || accobj->output == ACCOUT_MAX )
		data = uni_data_create_float(accobj->value);
	    else if(accobj->output == ACCOUT_LEVEL){
		data = uni_data_create_float(accobj->value/accobj->interval);
	    } else {
		data = uni_data_create_flow(accobj->value,  1000*accobj->interval);
	    }
	} else {
	    if(accobj->output == ACCOUT_SUM||accobj->output == ACCOUT_MIN || accobj->output == ACCOUT_MAX )
		data = uni_data_create_float(accobj->value);
	    else
		data = uni_data_create_float(accobj->value/accobj->count);
	}

	PRINT_MVB(&module->base, "Acc event %s avg %f data %d", accobj->event_type->name, uni_data_get_value(data), data->type);
	
	module_base_send_event(module_event_create(accobj->event_type, data, time));

	memcpy(&accobj->t_send, time, sizeof(accobj->t_send));
    } else {
	PRINT_MVB(&module->base, "Acc no event !!  count %d type %d",accobj->count , accobj->dtype );
    }
    accobj->count = 0;
    accobj->value = 0;
}


void* module_loop(void *parm)
{
    struct module_object *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;
    int retval;
    time_t prev_time;
    base->run = 1;
    
    while(base->run){ 
        struct acc_obj *accobj = this->accobjs;
        struct timeval tv;
	
        gettimeofday(&tv, NULL);
        while(accobj){
            if((tv.tv_sec%accobj->interval)==0){
                acc_obj_send(this, accobj, &tv);
            }
            accobj = accobj->next;
        }
        prev_time = tv.tv_sec;
	modutil_sleep_nextsec(&tv);
	PRINT_MVB(base, "looped :)");
    } 
    
    PRINT_MVB(base, "loop returned");
    
    return NULL;

}


struct event_handler *module_subscribe(struct module_base *module, struct module_base *source, struct event_type *event_type,
                     const char **attr)
{
    struct module_object *this = module_get_struct(module);
    struct acc_obj *new = acc_obj_create(attr);
    struct event_handler *event_handler = NULL;
    const char *unit   = get_attr_str_ptr(attr, "unit");

    char ename[64];
    if(strcmp(event_type->name, "fault")==0)
	return NULL;
    PRINT_MVB(module, "1");
    sprintf(ename, "%s.%s", source->name, event_type->name);

    new->interval = get_attr_int(attr, "interval", this->interval);
    PRINT_MVB(module, "2");
    new->event_type = event_type_create_copypart(&this->base, NULL, attr , event_type);
    PRINT_MVB(module, "3");
    event_handler = event_handler_create(event_type->name,  handler_rcv ,&this->base, (void*)new);
    PRINT_MVB(module, "4");
    this->base.event_handlers = event_handler_list_add(this->base.event_handlers, event_handler);
    PRINT_MVB(module, "5");
    this->base.event_types = event_type_add(this->base.event_types, new->event_type); // make it avaliable
    PRINT_MVB(module, "6");
    this->accobjs = acc_obj_add(this->accobjs, new);
    PRINT_MVB(module, "subscribe %s %p output=%s, interval %d", ename,event_handler, outtyps[new->output] ,new->interval );
    return event_handler;
}


struct module_type module_type = {    
    .name       = "accumulate",       
    .xml_tags   = module_tags,                      
    .handlers   = handlers ,                        
    .type_struct_size = sizeof(struct module_object), 
};


struct module_type *module_get_type()
{
    return &module_type;
}


/**
 * @} 
 */

/**
 * @} 
 */
