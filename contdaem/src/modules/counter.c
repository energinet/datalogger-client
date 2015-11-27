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
#include "module_value.h"

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


#define DEFAULT_PATH "/jffs2/counter"
#define DEFAULT_INTERVAL (300)

/**
 * @addtogroup module_xml
 * @{
 * @section countermodxml Flow counter module
 * Module for counting and storing a counter in a file
 * <b>Typename: "counter" </b>\n
 * <b> Tags: </b>
 * <ul>
 * <li><b>count:</b> 
 * </ul>
 * Example
 @verbatim 
  <module type="counter" name="counters" path="/jffs2/counter" verbose="false" flags="show" winterval="300">
    <counter name="power" text="Relæ 2" unit="kWh" event="" setvalue="cmdvars.adjpower" />
    <counter name="water" text="Relæ 2" unit="m³" adjust="cmdvars.adjwater" />
  </module>
 @endverbatim

 
 W (avg*time)--> kWh
 _J (add) --> kWh
 
 
*/


struct counter{
    struct evalue *evalue;
    struct convunit *convunit;
    struct timeval lastupdate;
    char *id;
    struct counter *next;
};

struct counter_module{
    struct module_base base;
    char *path;
    FILE *file; 
    int interval;
    time_t lastwrite;
    struct counter *counters;
};


struct counter_module* module_get_struct(struct module_base *module){
    return (struct counter_module*) module;
}


struct counter *counter_create(struct module_base *base, const char **attr, struct event_type *source)
{
    struct counter *new = malloc(sizeof(*new));
    assert(new);

    const char *unit    = get_attr_str_ptr(attr, "unit");
    const char *id      = get_attr_str_ptr(attr, "id");
    const char *name    = get_attr_str_ptr(attr, "name");
    const char *text    = get_attr_str_ptr(attr, "text");
    unsigned long flags = event_type_get_flags(get_attr_str_ptr(attr, "flags"), base->flags);
    
    fprintf(stderr, "dsankjdnkjnsakj sa\n");

    if(unit){
	char unit_out[64];
	snprintf(unit_out, sizeof(unit_out), "_%s", unit);
	new->convunit = module_conv_get_flow(source->unit, unit_out);
    } else {
	new->convunit = module_conv_get_flow(source->unit, NULL); 
	if(new->convunit){
	    if(new->convunit->unit_out[0] == '_')
		unit = new->convunit->unit_out + 1; 
	    else
		unit = new->convunit->unit_out; 
	}
    }

    if(!name)
	name = source->name;

    if(!text)
	text = source->hname;

    if(!id)
	id = source->name;

    new->id = strdup(id);

    new->evalue = evalue_create_ext(base, name,  NULL,  NULL, flags, unit, text);
    base->event_types = event_type_add(base->event_types, new->evalue->event_type);

    new->next = NULL;

    new->lastupdate.tv_sec = 0;
    new->lastupdate.tv_usec = 0;

    return new;
}

struct counter *counter_lst_add(struct counter *list, struct counter *new)
{
    struct counter *ptr = list;
    
    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}

struct counter *counter_lst_find(struct counter *list, const char *id)
{
    struct counter *ptr = list;
	
    while(ptr){
	if(strcmp(ptr->id, id )==0)
	    return ptr;
        ptr = ptr->next;
    }

    return NULL;
}



void counter_delete(struct counter *counter)
{
    if(!counter)
	return;
    
    counter_delete(counter->next);
    evalue_delete(counter->evalue);
    free(counter);
}

void counter_update(struct counter *counter, float value, struct timeval *now)
{

    unsigned long msec = 0;

    if(counter->lastupdate.tv_sec != 0)
	msec = modutil_timeval_diff_ms(now, &counter->lastupdate);

    float _value  = evalue_getf(counter->evalue);
    
    fprintf(stderr, "update +%f, %f > ", value, _value);

    _value +=  module_conv_calc(counter->convunit,  msec, value);
      
    fprintf(stderr, "%f\n", _value);

    evalue_setft(counter->evalue, _value, now);
    
    counter->lastupdate.tv_sec = now->tv_sec;
    counter->lastupdate.tv_usec = now->tv_usec;   
        
}

void counter_read(struct counter *counter, const char *str)
{
    float value = 0;
    struct timeval lastupdate;
    memset(&lastupdate, 0, sizeof(lastupdate));
    
    sscanf(str, "%f;%ld",  &value , &lastupdate.tv_sec);
	   
    evalue_setft(counter->evalue, value, &lastupdate);

}


int counter_write(struct counter *counter, char *buf, int maxlen)
{
    return snprintf(buf, maxlen, "%f;%ld", evalue_getf(counter->evalue), counter->lastupdate.tv_sec);

}


int handler_rcv(EVENT_HANDLER_PAR)
{
    struct counter *counter = (struct counter *)handler->objdata;
    struct uni_data *data = event->data;
    float value =  uni_data_get_value(data);
    
    counter_update(counter, value, &event->time);

    return 0;
}

int handler_adj(EVENT_HANDLER_PAR)
{
    struct counter *counter = (struct counter *)handler->objdata;
    struct uni_data *data = event->data;
    float value =  uni_data_get_value(data);
    
    evalue_setft(counter->evalue, value, &event->time);
    
    return 0;
}



int xml_start_counter(XML_START_PAR)
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




struct xml_tag module_tags[] = {
    { "counter"  , "module" , xml_start_counter, NULL, NULL},
    { "" , ""  , NULL, NULL, NULL }
};
  



int handler_def(EVENT_HANDLER_PAR)
{
    return 0;
}


struct event_handler handlers[] = { {.name = "def", .function = handler_def}, \
				    {.name = "rcv", .function = handler_rcv} , \
				    {.name = "adj", .function = handler_adj} ,{.name = ""}};

  
int module_init(struct module_base *module, const char **attr)
{
    struct counter_module* this = module_get_struct(module);

    const char *path = get_attr_str_ptr(attr, "path");

    if(!path){
	path = DEFAULT_PATH;
    }

    this->path = strdup(path);

    this->file = NULL;

    this->interval = get_attr_int(attr, "interval", DEFAULT_INTERVAL);

    this->lastwrite = 0;

    this->counters = NULL;

    return 0;
}



void module_deinit(struct module_base *module)
{
    struct counter_module* this = module_get_struct(module);

    counter_delete(this->counters);
    
    fclose(this->file);

    return;
}


int module_init_read(struct counter_module *this){

    char buf[512];
    struct timeval now;

    PRINT_MVB(&this->base, "init counters");


    gettimeofday(&now, NULL);    

    rewind(this->file);

    while (fgets(buf, sizeof(buf), this->file)) {
	char *val = strchr(buf,'=');
	
	if(!val)
	    break;
	
	val[0]='\0';

	PRINT_MVB(&this->base, "search id %s", buf);


	struct counter *counter = counter_lst_find(this->counters, buf);
	
	if(!counter)
	    continue;

	PRINT_MVB(&this->base, "init counter %s to %s", counter->id, val+1);


	counter_read(counter, val+1);
    }

    return 0;
}


int module_write(struct counter_module *this)
{
    struct counter *ptr = this->counters;
    char buf[512];

    rewind(this->file);
    
    while(ptr){
	int len =  counter_write(ptr, buf, sizeof(buf));
	if(len < 0)
	    PRINT_ERR("len < 0 (%d)",len);
	fprintf(this->file, "%s=%s\n", ptr->id, buf);
	ptr = ptr->next;
    }

    fflush(this->file);
    
    return 0;
    
}

void* module_loop(void *parm)
{
    struct counter_module *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;
    struct timeval tv;

    base->run = 1;
  
    PRINT_MVB(base, "opening file ");
 
    this->file = fopen(this->path, "r+");
    
    if(this->file == NULL){
	PRINT_MVB(base, "Creating file");
	this->file = fopen(this->path, "w");
    }

    if(this->file == NULL){
	PRINT_ERR("ERROR opening file '%s': %s", this->path, strerror(errno));
	return NULL;
    }
    
    module_init_read(this);
    
    while(base->run){ 

	gettimeofday(&tv, NULL);

	if(tv.tv_sec > (this->lastwrite + this->interval)){
	    PRINT_MVB(base, "write :)");
	    module_write(this);
	    this->lastwrite = tv.tv_sec;
	}

	modutil_sleep_nextsec(&tv);
	PRINT_MVB(base, "looped :)");
    } 
    
    PRINT_MVB(base, "loop returned");
    
    return NULL;

}


struct event_handler *module_subscribe(struct module_base *base, struct module_base *source, struct event_type *event_type,
                     const char **attr)
{
    struct counter_module *this = module_get_struct(base);
    struct counter *counter = counter_create(base, attr, event_type);
    struct event_handler *event_handler = NULL;
    const char *adj = get_attr_str_ptr(attr, "adj");

    char ename[64];
    if(strcmp(event_type->name, "fault")==0)
	return NULL;

    sprintf(ename, "%s.%s", source->name, event_type->name);

    event_handler = event_handler_create(event_type->name,  handler_rcv ,&this->base, (void*)counter);
    base->event_handlers = event_handler_list_add(base->event_handlers, event_handler);


    if(adj){    
	struct event_type *event = module_base_get_event(base->first,adj, FLAG_ALL);
	
	event_handler = event_handler_create(event_type->name,  handler_adj ,&this->base, (void*)counter);
	base->event_handlers = event_handler_list_add(base->event_handlers, event_handler);
	module_base_subscribe(base, event, event_handler);
    }


    this->counters = counter_lst_add(this->counters, counter);
    PRINT_MVB(base, "subscribe %s %p", ename,event_handler);
    return event_handler;
}





struct module_type module_type = {    
    .name       = "counter",       
    .xml_tags   = module_tags,               
    .handlers   = handlers ,                               
    .type_struct_size = sizeof(struct counter_module), 
};


struct module_type *module_get_type()
{
    return &module_type;
}


