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
#include <assert.h>

struct planitem{
    int mn_sec;
    float outval;
    struct planitem *next;
};

time_t planitem_rd_time(const char *timestr)
{
    int hour = 0;
    int min  = 0;
    int sec  = 0; 

    if(!timestr){
	PRINT_ERR("Time string is NULL");
	return -2;
    }
	

    if(sscanf(timestr, "%d:%d:%d", &hour, &min, &sec)>=2)
	return sec + (min * 60) + (hour * 3600);
    else if(sscanf(timestr, "%d", &sec)==1)
	return sec;
	
    PRINT_ERR("Could not parse time string '%s'", timestr);

    return -1;

}




struct planitem* planitem_create(const char **attr)
{
    struct planitem *new = malloc(sizeof(*new));
    assert(new);

    const char *timestr   = get_attr_str_ptr(attr, "time");
    const char *outvalstr = get_attr_str_ptr(attr, "out");
    
    fprintf(stderr,"dsnajk\n");

    new->mn_sec = planitem_rd_time(timestr);

    if(new->mn_sec<0){
	goto err_out;
    }

    if(!outvalstr){
	PRINT_ERR("No out tag present in dayplan item");
	goto err_out;
    }

    if(sscanf(outvalstr, "%f", &new->outval)!=1){
	PRINT_ERR("Could not parse item output value string '%s'", outvalstr);
	goto err_out;
    }
    
    new->next = NULL;

    return new;
    
  err_out:
    free(new);
    return NULL;
}


struct planitem* planitem_add(struct planitem* list, struct planitem* new)
{
    struct planitem *ptr = list;
    
    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}

void planitem_delete(struct planitem *item)
{
    if(!item)
	return;

    planitem_delete(item->next);

    free(item);
}


struct planitem* planitem_get_cuttent(struct planitem* list, time_t now)
{
    
    struct tm tm_now;
    int mn_sec;
    int min_diff = 10000000;
    struct planitem* selected = NULL;
    struct planitem* ptr = list;
    gmtime_r(&now, &tm_now);
       
    mn_sec = tm_now.tm_sec + (tm_now.tm_min * 60) + (tm_now.tm_hour * 3600);

    while(ptr){
	time_t diff = mn_sec - ptr->mn_sec;
	if(diff <0)
	    diff += (60*60*24);
	
	if(diff <= min_diff){
	    min_diff = diff;
	    selected = ptr;
	}
	    
	ptr = ptr->next;
    }

    return selected;

}

struct dayplan_object{
    struct module_base base;
    struct planitem *current;
    struct planitem *plan;
    struct module_tick *tick;
    struct event_type *event_type;
    struct timeval last_upd;
};


struct dayplan_object* module_get_struct(struct module_base *base){
    return (struct dayplan_object*) base;
}


int dayplan_send_event(struct dayplan_object* this, 
				struct timeval *now, float value)
{
    /* int diff = 0; */
    /* if(this->last_upd.tv_sec != 0) */
    /* 	 diff = modutil_timeval_diff_ms(now, &this->last_upd); */

    struct uni_data *data = uni_data_create_float(value);
    PRINT_MVB(&this->base, "Sending event value %f ", value);
    module_base_send_event(module_event_create(this->event_type, data,
					       now));

    memcpy(&this->last_upd, now, sizeof(struct timeval));

    return 0;
}


int handler_timeset(EVENT_HANDLER_PAR)
{
    struct dayplan_object* this = module_get_struct(handler->base);
    struct planitem *item = (struct planitem*)handler->objdata;
    struct uni_data *data = event->data;
    const char *timestr = uni_data_get_strptr(data);
    int mn_sec = -1;

    PRINT_MVB(&this->base,"Received time : %s \n",timestr); 

    mn_sec =  planitem_rd_time(timestr);

    if(mn_sec<0){
	return mn_sec;
    }
	    
    item->mn_sec = mn_sec;
    
    PRINT_MVB(&this->base,"Set midnight set to %d\n",item->mn_sec); 

    return 0;
}


int handler_value(EVENT_HANDLER_PAR)
{
    struct dayplan_object* this = module_get_struct(handler->base);
    struct planitem *item = (struct planitem*)handler->objdata;
    struct uni_data *data = event->data;
    float value = uni_data_get_value(data);
 
    
    PRINT_MVB(&this->base,"Received value %f for time %2.2d:%2.2d:%2.2d \n",
	      value,item->mn_sec/3600, (item->mn_sec/60)%60, item->mn_sec%60 ); 

    
    item->outval = value;

    if(this->current == item){
	dayplan_send_event(this, &event->time, item->outval);
    }
	
	

    return 0;
   
}


int module_regsub(struct module_base *base, void *objdata, 
		  const char *handlername, struct modules *modules,
		  int (*function)(EVENT_HANDLER_PAR), const char* subscribe, 
		  const char **attr)
{
    
    
    struct event_handler* handler = event_handler_create(handlername, function
				       ,base, objdata);

    base->event_handlers = event_handler_list_add(base->event_handlers
						  , handler);

    if(subscribe)
	return module_base_subscribe_event(base, modules->list, 
					   subscribe, 
					   FLAG_ALL,  
					   handler, 
					   attr);

    return 0;

}
		  


int start_planitem(XML_START_PAR)
{

    
    struct modules *modules = (struct modules*)data;
    struct module_base *base = (struct module_base*)ele->parent->data;
    PRINT_MVB(base,"start_planitem");
    struct dayplan_object* this = module_get_struct(base);

    PRINT_MVB(base,"start_planitem 2");
    struct planitem *item =  planitem_create(attr);

    if(!item)
	return -1;
    
    PRINT_MVB(base,"start_planitem 3");
    this->plan = planitem_add(this->plan, item);

    module_regsub(base, (void*)item, "val",modules,  handler_value , 
		  get_attr_str_ptr(attr, "eventval"), attr);

    module_regsub(base, (void*)item, "time",modules,  handler_timeset , 
		  get_attr_str_ptr(attr, "eventtime"), attr);

    PRINT_MVB(base,"Created plan item  value %f for time %2.2d:%2.2d:%2.2d \n",
	      item->outval,item->mn_sec/3600, 
	      (item->mn_sec/60)%60, item->mn_sec%60 );

    return 0;
}



struct xml_tag module_tags[] = {                         \
    { "plan"  , "module" , start_planitem, NULL, NULL},   \
    { "" , ""  , NULL, NULL, NULL }                       \
};



int module_init(struct module_base *base, const char **attr)
{
    struct dayplan_object* this = module_get_struct(base);
    
    this->tick = module_tick_create(base->tick_master,  base, 1, TICK_FLAG_SECALGN1);

    this->event_type =  event_type_create_attr(base,NULL,attr);
    base->event_types = event_type_add(base->event_types, this->event_type);

    return 0;
}

void module_deinit(struct module_base *base)
{
    struct dayplan_object* this = module_get_struct(base);
    
    planitem_delete(this->plan);
    module_tick_delete(this->tick);

    return;
}


void* module_loop(void *parm)
{ 
    struct module_base *base = ( struct module_base *)parm;
    struct dayplan_object* this = module_get_struct(base);
    struct timeval now;

    base->run = 1;
    PRINT_MVB(base, "start loop\n");

    while(base->run){ 
		module_tick_wait(this->tick, &now);
		struct planitem* current = planitem_get_cuttent(this->plan,now.tv_sec);
		
		if(current != this->current){
			dayplan_send_event(this, &now, current->outval);
			this->current = current;
		}
	    
    } 
    
    PRINT_MVB(base, "loop returned");
    
    return NULL;

}



struct module_type module_type = {                  \
    .name       = "dayplan",                       \
    .xml_tags   = module_tags,                      \
    .type_struct_size = sizeof(struct dayplan_object), \
};


struct module_type *module_get_type()
{
    return &module_type;
}



