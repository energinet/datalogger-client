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


/**
 * @ingroup modules 
 * @{
 */

/**
 * @defgroup modules_relay Module for setting relays
 * @{
 */


/**
 * Rlay object struct
 */
struct relay {
    char *path;
    int num;
    struct event_type *event_type;
    struct relay *next;
};


/**
 * Relay module struct 
 */
struct relays_object{
    struct module_base base;
    char *basepath;
    int num_next;
    struct relay *relays;
};

struct relays_object* module_get_struct(struct module_base *base){
    return (struct relays_object*) base;
}


/**
 * Create relay object 
 */
struct relay *relay_create(void)
{
    struct relay *new = malloc(sizeof(*new));
    assert(new);
    
    memset(new, 0, sizeof(*new));

    return new;
    
}

int relay_rep_num(void *usedata, char* buffer, int max_size)
{
    int *num = (int*)usedata;

//    fprintf(stderr, "test hsaj %p\n", usedata);

//    fprintf(stderr, "test hsaj %d\n", *num);

    return snprintf(buffer, max_size, "%d",  *num);
}

struct mureplace_obj repl_list[] = {	\
    { "%num%" , relay_rep_num },	\
    { NULL , NULL }			\
};



struct relay *relay_add(struct relay *first, struct relay *new)
{
    struct relay *ptr = first;

    if(!first)
        /* first module in the list */
        return new;

    /* find the last module in the list */
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return first;
}

void relay_delete(struct relay *this)
{
    if(!this)
	return;
    
    relay_delete(this->next);

    free(this);
}

void relay_send_upd(struct relay *this, int value)
{
    if(!this->event_type)
	return ;
    
    module_base_send_event(
	module_event_create(
	    this->event_type, 
	    uni_data_create_int(value),  
	    NULL) );   
}

int relay_output_set(struct relay *this, int value)
{
    const char *path = this->path;
    FILE *file = NULL;
    
    if(!path){
        PRINT_ERR("no device");
        return -EFAULT;
    }

    file = fopen(path, "w");

    if(!file){
        PRINT_ERR("error opening %s: %s", path, strerror(errno));
        return -errno;
    }

    fprintf(file,"%d", value);  

    fclose(file);
    
    return 0;
}

int relay_output_get(struct relay *this)
{
    const char *path = this->path;
    int value = -EFAULT;
    FILE *file = NULL;

    if(!path){
        PRINT_ERR("no device");
        return -EFAULT;
    }

    file = fopen(path, "r");

    if(!file){
        PRINT_ERR("error opening %s: %s", path, strerror(errno));
        return -errno;
    }

    fscanf(file,"%d", &value);  

    fclose(file);
    
    
    return value;
}

int relay_set(struct relay *this, struct uni_data *data)
{
    int set = 0;
    int retval = 0;
    
    if(data->value)
	set = 1;

    retval = relay_output_set(this, set);
    relay_send_upd(this, set);
    
    return retval;
}


int handler_rcv(EVENT_HANDLER_PAR)
{
    struct relays_object* this = module_get_struct(handler->base);
    struct relay *relay = (struct relay*)handler->objdata;
    
    return relay_set(relay, event->data);

}

/**
 * Read event handler
 */
struct module_event *modbus_eread(struct event_type *event)
{
     struct relay *relay = (struct relay *)event->objdata;
     struct relays_object* this = module_get_struct(event->base);
     
     int val = relay_output_get(relay);

     return module_event_create(event, uni_data_create_int(val), NULL);
}

/**
 * Write event handler
 */
int modbus_ewrite(struct event_type *event, struct uni_data *data)
{
     struct relay *relay = (struct relay *)event->objdata;

     return relay_set(relay, data);
}


struct event_handler handlers[] = { {.name = ""}};

int start_device(XML_START_PAR)
{
    struct relays_object* this = module_get_struct(ele->parent->data);
    const char *text = get_attr_str_ptr(attr, "device"); 
    const char *listen = get_attr_str_ptr(attr, "listen"); 

    if(!text){
        PRINT_ERR("no text");
        return -EFAULT;
    }
        
    this->basepath = strdup(text);  

    return 0;
}

  /* <module type="relay" name="relays" basepath=--+"/sys/class/leds/relay-X%num%/brightness" verbose="1"> */
  /*   <output device="/sys/class/leds/relay-X%num%/brightness"/> */
  /*   <relay num="1" event="thr.gwpump" /> */
  /* </module> */

int start_relay(XML_START_PAR)
{
    struct modules *modules = (struct modules*)data;
    struct module_base* base = ele->parent->data;
    struct relays_object *this = module_get_struct(base);
    struct event_handler *handler = NULL;
    struct relay *new = relay_create();
    const char *path = get_attr_str_ptr(attr, "path");  
    new->num = get_attr_int(attr, "num", this->num_next );  

    PRINT_MVB(base, "Addeding relay %d ", new->num);

    if(path){
	new->path = strdup(path);	
    } else if(!new->path && this->basepath && new->num >= 0){
	char tmppath[256];
	this->num_next++;
	PRINT_MVB(base, "Base path %s ", this->basepath);
	if(moduleutil_replace(this->basepath, tmppath, sizeof(tmppath), repl_list, &new->num)>0){
	    new->path = strdup(tmppath);    
	}
    }

    PRINT_MVB(base, "Addeding relay %s ", new->path);

    this->relays = relay_add(this->relays, new);

    handler = event_handler_create("rcv", handler_rcv ,base, new);  
    base->event_handlers = event_handler_list_add(base->event_handlers, handler);
    
    new->event_type = event_type_create_attr(base, new, attr);
    
    if(new->event_type){
	new->event_type->read = modbus_eread;
	new->event_type->write = modbus_ewrite;
    }
    
    base->event_types = event_type_add(base->event_types, new->event_type );
    
    PRINT_MVB(base, "Added relay %d %s ", new->num, new->path  );

    if(get_attr_str_ptr(attr, "event"))
	return module_base_subscribe_event(base, modules->list, get_attr_str_ptr(attr, "event"), FLAG_ALL,  handler, attr);
    else
	return 0;
    
}


struct xml_tag module_tags[] = {                         \
    { "relay" , "module" , start_relay, NULL, NULL},	 \
    { "listen" , "module" , start_relay, NULL, NULL},	 \
    { "output" , "module" , start_device, NULL, NULL}, \ 
    { "" , "" , NULL, NULL, NULL }                       \
};
    



int module_init(struct module_base *module, const char **attr)
{
    struct relays_object* this = module_get_struct(module);
    
    this->basepath = get_attr_str_ptr(attr, "basepath");
    
    this->num_next = 1;

    PRINT_DBG(1, "module %s", this->base.name);

    return 0;
}


struct module_type module_type = {                  \
    .name       = "relay",                          \
    .xml_tags   = module_tags,                      \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct relays_object), \
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
