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


struct module_object{
    struct module_base base;
    char *binpath;
    char *args;
};

struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}

int event_handler_all(struct module_event *event, void* userdata)
{
    struct module_object* this = module_get_struct(userdata);
    struct uni_data *data = event->data;

    return 0;
}

struct event_handler handlers[] = { {.name = "all", .function = event_handler_all} ,{.name = ""}};
struct event_type events[] = { {.name = "fault"}, {.name = ""}};

int start_bin(XML_START_PAR)
{
    struct module_object* this = module_get_struct(ele->parent->data);
    const char *execpath = get_attr_str_ptr(attr, "path"); 
    const char *args = get_attr_str_ptr(attr, "args"); 
    
    if(!execpath||!args){
        PRINT_ERR("no text");
        return -EFAULT;
    }
        
    this->args = strdup(args);
    this->binpath = strdup(execpath);

    return 0;
}

struct xml_tag module_tags[] = {                         \
    { "bin" , "module" , start_bin, NULL, NULL},   \
    { "" , "" , NULL, NULL, NULL }                       \
};
    

int module_init(struct module_base *module, const char **attr)
{
    struct module_object* this = module_get_struct(module);
    
    this->binpath = NULL
    this->args = NULL;

    PRINT_DBG(1, "module %s", this->base.name);

    return 0;
}


void module_deinit(struct module_base *module) 
{
    struct module_object* this = module_get_struct(module);

    PRINT_DBG(1, "module %s", this->base.name);

    return;
}


int module_start(struct module_base *module)
{
    struct module_object* this = module_get_struct(module);

    PRINT_DBG(1, "module %s", module->name);

    if(this->filepath)
        db_sqlite_open(&this->db_obj, this->filepath);    
    else
        return -EFAULT;
    

    return 0;
}

void* module_loop(void *parm)
{
    struct module_object *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;

    base->run = 1;
    
    while(base->run){ 
         sleep(10); 
         // PRINT_DBG(1, "loop"); 
     } 

     PRINT_DBG(1, "loop returned");
    
     return NULL;

}

struct module_type module_type = {                  \
    .name       = "dblogger",                         \
    .mtype      = MTYPE_DATALOG,                    \
    .subtype    = SUBTYPE_NONE,                     \
    .init       = module_init,                      \
    .start      = module_start,                     \
    .deinit     = module_deinit,                    \
    .loop       = module_loop,                      \
    .xml_tags   = module_tags,                      \
    .events     = events,                           \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct module_object), \
};




struct module_type *module_get_type()
{
    return &module_type;
}
