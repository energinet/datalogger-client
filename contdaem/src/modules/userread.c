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


struct module_object{
    struct module_base base;
    struct callback_list* readlist;
};

struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}


struct xml_tag module_tags[] = {                         \
    { "" , "" , NULL, NULL, NULL }                       \
};

struct event_handler handlers[] = {{.name = ""}};
    
int module_init(struct module_base *module, const char **attr)
{
    struct module_object* this = module_get_struct(module);

    this->readlist = NULL;
	
	return 0;
}


int module_start(struct module_base *module)
{
    struct module_object* this = module_get_struct(module);
    struct module_base *ptr = module->first;
    int i= 0, n= 0;


    while(ptr){
	struct event_handler *handler = event_handler_get(ptr->event_handlers, "read");
	if(handler){
	    i++;
	    PRINT_MVB(module, "Added %s", ptr->name);
	    this->readlist = callback_list_add(this->readlist, callback_list_create(handler));
	} 
	n++;
	ptr= ptr->next;
    }

    PRINT_MVB(module, "found %d/%d %p", i, n, this->readlist);
    return 0;
}


void module_deinit(struct module_base *module)
{
    struct module_object* this = module_get_struct(module);

    callback_list_delete(this->readlist);
    return;
}


void module_print_reads(struct module_event* event)
{
                        
    printf("%s\n", "___________________________________________________________________");
    while(event){
	char buf[1024];
	int len = 0;
	len += module_event_sprint(event, buf, sizeof(buf));
	printf("%s\n", buf);
	event = event->next;
    }

    
}


void* module_loop(void *parm)
{
	struct module_base *base = ( struct module_base *)parm;
        
    base->run = 1;
    
    while(base->run){ 
	struct module_event* event = module_base_read_all(base->first, NULL, FLAG_ALL );
//	module_event_create(NULL,NULL, NULL); 
//	callback_list_run(this->readlist, event);
	module_print_reads(event);
	module_event_delete(event);
	sleep(3);
    } 
    
    PRINT_MVB(base, "loop returned");
    
    return NULL;

}

struct module_type module_type = {                  \
    .name       = "userread",                       \
    .mtype      = MTYPE_OUTPUT,                    \
    .subtype    = SUBTYPE_LEVEL,                     \
    .xml_tags   = NULL ,                      \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct module_object), \
};


struct module_type *module_get_type()
{
    return &module_type;
}
