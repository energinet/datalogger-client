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

struct module_template{
    struct module_base base;
    int value;
    char *text;
};


struct module_template* module_get_struct(struct module_base *module){
    return (struct module_template*) module;
}


int event_handler_all(EVENT_HANDLER_PAR)
{
    struct module_template* this = module_get_struct(handler->base);
    printf("Module type %s received event\n", this->base.name);
    return 0;
}

struct event_handler handlers[] = { {.name = "all", .function = event_handler_all} ,{.name = ""}};
struct event_type events[] = { {.name = "data"} , {.name = "fault"}, {.name = ""}};


int start_value(XML_START_PAR)
{
    struct module_template* this = module_get_struct(ele->parent->data);
    this->value = get_attr_int(attr, "set", this->value );
    return 0;
}


int start_text(XML_START_PAR)
{
    struct module_template* this = module_get_struct(ele->parent->data);
    const char *text = get_attr_str_ptr(attr, "set"); 

    if(!text){
        PRINT_ERR("no text");
        return -EFAULT;
    }
        
    this->text = strdup(text);

    return 0;
}

struct xml_tag module_tags[] = {                         \
    { "value" , "module" , start_value, NULL, NULL},   \
    { "text" , "module" , start_text, NULL, NULL}, \
    { "" , "" , NULL, NULL, NULL }                       \
};
    

int module_init(struct module_base *module, const char **attr)
{
    struct module_template* this = module_get_struct(module);
    
    this->value = -1;   
    this->text = NULL;

    PRINT_DBG(1, "module %s", this->base.name);

    return 0;
}


void module_deinit(struct module_base *module)
{
    struct module_template* this = module_get_struct(module);

    PRINT_DBG(1, "module %s", this->base.name);

    return;
}


int module_start(struct module_base *module)
{
    struct module_template* this = module_get_struct(module);

    PRINT_DBG(1, "module %s", this->base.name);

    return 0;
}

void* module_loop(void *parm)
{
    struct module_base *base = ( struct module_base *)parm;

    base->run = 1;
    
    while(base->run){ 
         sleep(1); 
         PRINT_DBG(1, "loop"); 
     } 

     PRINT_DBG(1, "loop returned");
    
     return NULL;

}

struct module_type module_type = {                  
    .name       = "template",                       
    .mtype      = MTYPE_DATALOG,                    
    .subtype    = SUBTYPE_NONE,                     
    .init       = module_init,                      
    .start      = module_start,                     
    .deinit     = module_deinit,                    
    .loop       = module_loop,                      
    .xml_tags   = module_tags,                      
    .handlers   = handlers ,                        
    .type_struct_size = sizeof(struct module_base), 
};


struct module_type *module_get_type()
{
    return &module_type;
}
