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
};

int get_module_event_txt_name(struct module_event* event, char* buffer, int max_size)
{
    return snprintf(buffer, max_size, "%s.%s", event->source->name, event->type->name);
}



struct replace_obj{
    char *search;
    int (*event)(struct module_event* event, char* buffer, int max_size);
    int (*data)(struct uni_data *data, char* buffer, int max_size);
};

struct replace_obj repl_list[] = {                              \
    { "%data.time%" , module_event_get_txt_time , NULL },           \
    { "%data.value%" , NULL , uni_data_get_txt_value },         \
    { "%event.name%" , get_module_event_txt_name, NULL },       \
    { NULL , NULL, NULL }                                       \
};
    

struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}

int event_handler_all(struct module_event *event, void* userdata)
{
    struct module_object* this = module_get_struct(userdata);
    struct uni_data *data = event->data;


    char cmd[512];
    int ptr = 0;
    char *in_ptr = this->binpath;
    char *in_ptr_end = in_ptr + strlen(in_ptr);
    char *in_ptr_next;
    int repl_index = -1;
    int i = 0;

    if(!in_ptr)
        return -1;

    while(in_ptr < in_ptr_end){
        int cpy_size = 0;
        /* find first replace var */
        in_ptr_next = in_ptr_end;
        i = 0;
        repl_index = -1;
        while(repl_list[i].search != NULL){
            char *tmp_ptr = strstr(in_ptr, repl_list[i].search);
            if(tmp_ptr)
                if(tmp_ptr < in_ptr_next){
                    in_ptr_next = tmp_ptr;
                    repl_index = i;
                }
            i++;
        }
        
        cpy_size = in_ptr_next - in_ptr;


        memcpy(cmd+ptr, in_ptr,  in_ptr_next-in_ptr);


        in_ptr +=  cpy_size;
        ptr += cpy_size;
        if( repl_index > -1){
            if(repl_list[repl_index].data)
                ptr += repl_list[repl_index].data(data, cmd+ptr,  512-ptr);
            else if(repl_list[repl_index].event)
                ptr += repl_list[repl_index].event(event, cmd+ptr,  512-ptr);
            in_ptr += strlen(repl_list[repl_index].search);
        }

    }
    
    cmd[ptr] = '\0';

    PRINT_DBG(this->base.verbose, "cmd: %s\n", cmd);

    system(cmd);

    return 0;
}

struct event_handler handlers[] = { {.name = "all", .function = event_handler_all} ,{.name = ""}};
struct event_type events[] = { {.name = "fault"}, {.name = ""}};

int start_exec(XML_START_PAR)
{
    struct module_object* this = module_get_struct(ele->parent->data);
    const char *execpath = get_attr_str_ptr(attr, "cmd"); 
    
    if(!execpath){
        PRINT_ERR("no text");
        return -EFAULT;
    }
        
    this->binpath = strdup(execpath);

    return 0;
}

struct xml_tag module_tags[] = {                         \
    { "exec" , "module" , start_exec, NULL, NULL},   \
    { "" , "" , NULL, NULL, NULL }                       \
};
    

int module_init(struct module_base *module, const char **attr)
{
    struct module_object* this = module_get_struct(module);
    
    this->binpath = NULL;
    
    PRINT_DBG(this->base.verbose, "module %s", this->base.name);

    return 0;
}


void module_deinit(struct module_base *module) 
{
    struct module_object* this = module_get_struct(module);

    PRINT_DBG(this->base.verbose, "module %s", this->base.name);

    return;
}


int module_start(struct module_base *module)
{
    struct module_object* this = module_get_struct(module);

    PRINT_DBG(this->base.verbose, "module %s", module->name);


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

     PRINT_DBG(this->base.verbose, "loop returned");
    
     return NULL;

}

struct module_type module_type = {                  \
    .name       = "bashout",                         \
    .mtype      = MTYPE_OUTPUT,                    \
    .subtype    = SUBTYPE_EVENT,                     \
    .init       = module_init,                      \
    .start      = NULL /*module_start*/,            \
    .deinit     = NULL /*module_deinit*/,           \
    .loop       = NULL /*module_loop*/,             \
    .xml_tags   = module_tags,                      \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct module_object), \
};
//    .events     = events,                     \




struct module_type *module_get_type()
{
    return &module_type;
}
