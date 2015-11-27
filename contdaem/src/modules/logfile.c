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
#define DEFAULT_LOG_FILE "/jffs2/logfile.csv"


struct module_object{
    struct module_base base;
    char *filepath;
    FILE *stream;
    int show_hidden;
    pthread_mutex_t mutex;
    struct module_event* fifo_list;
};


struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}





int event_handler_all(EVENT_HANDLER_PAR)
{
    struct module_object* this = module_get_struct(handler->base);
    struct uni_data *data = event->data;
    struct tm tm; 
    
    char entry_txt[512];

    pthread_mutex_lock(&this->mutex);
   
    gmtime_r(&event->time.tv_sec, &tm);
    uni_data_get_txt_value(data, entry_txt, sizeof(entry_txt));

    fprintf(this->stream, "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d.%3.3ld, %s.%s, %s\n", 
	    tm.tm_year + 1900, tm.tm_mon+1, tm.tm_mday, 
	    tm.tm_hour, tm.tm_min, tm.tm_sec, event->time.tv_usec/1000,
	    event->source->name, event->type->name,
	    entry_txt);
    
    fflush(this->stream);

    pthread_mutex_unlock(&this->mutex);

    return 0;

}

struct event_handler handlers[] = { {.name = "log", .function = event_handler_all} ,{.name = ""}};
struct event_type events[] = { {.name = "fault"}, {.name = ""}};

int start_log(XML_START_PAR)
{
    struct modules *modules = (struct modules*)data;
    struct module_base* this = ele->parent->data;
    struct event_handler* handler = NULL;
    unsigned long mask;
    if(!this)
	return 0;

    mask = event_type_get_flags(get_attr_str_ptr(attr, "flags"), FLAG_LOG);

    handler = event_handler_get(this->event_handlers, "log");

    assert(handler); 

    return module_base_subscribe_event(this, modules->list, get_attr_str_ptr(attr, "event"),mask,  handler, attr);
}


struct xml_tag module_tags[] = {                         \
    { "log", "module", start_log, NULL, NULL},		 \
    { "" , "" , NULL, NULL, NULL }                       \
};
    

int module_init(struct module_base *module, const char **attr)
{
    struct module_object* this = module_get_struct(module);
    const char *file = get_attr_str_ptr(attr, "file"); 

    this->show_hidden = get_attr_int(attr, "show_hidden", 0); 
    
    if(file)
        this->filepath = strdup(file);
    else 
        this->filepath = DEFAULT_LOG_FILE;

    this->stream  = fopen(this->filepath, "a");
     
    pthread_mutex_init(&this->mutex, NULL);
    
    return 0;
}


void module_deinit(struct module_base *module) 
{
    struct module_object* this = module_get_struct(module);

    pthread_mutex_destroy(&this->mutex);
    
    fclose(this->stream);

    return;
}




struct event_handler *module_subscribe(struct module_base *module, struct module_base *source, struct event_type *event_type,
                     const char **attr)
{
    struct module_object *this = module_get_struct(module);
    struct event_handler* handler = NULL;
    char ename[64];
    if(strcmp(event_type->name, "fault")==0)
	return NULL;

    sprintf(ename, "%s.%s", source->name, event_type->name);
    PRINT_MVB(module, "subscribe %s", ename);
    
    

    if(!(event_type->flags & FLAG_SHOW)&&(!this->show_hidden))
	return NULL;


    handler = event_handler_get(this->base.event_handlers, "log");
    return handler;


}

struct module_type module_type = {                  \
    .name       = "csvlogger",                         \
    .mtype      = MTYPE_DATALOG,                    \
    .subtype    = SUBTYPE_NONE,                     \
    .xml_tags   = module_tags,                      \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct module_object), \
};

struct module_type *module_get_type()
{
    return &module_type;
}
