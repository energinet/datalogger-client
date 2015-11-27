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



/**
 * @addtogroup module_xml
 * @{
 * @section antibouncmodxml Antibounce  module
 * Module for removing chattering
 * <b>Typename: "antibounce" </b>\n
 * <b> Tags: </b>
 * <ul>
 * <li><b>chatrem:</b> Chatter removal of one variable
 * - <b> delay:</b> Miliseconds to wait before switch (tick time is 100 ms)\n
 * - <b> copytime:</b> Set true if time should be copyed from input.
 * </ul>
 @}
*/


/**
 * Antibounce object */
struct abounce{
    /**
     * The delay to wait before transmitting change. */
    int delay;
    /**
     * Indicate if the value has been changed since last transmitted value. */
    int updated;
    /**
     * Copy the time from the input. */
    int copytime;
    /**
     * The input evalue */
    struct evalue *input;
    /** 
     * The output evalue */
    struct evalue *output;
    /**
     * The next object in the list. NULL if the end */
    struct abounce *next;
};

/**
 * antibounce module */
struct abounce_module{
    /**
     * The module base */
    struct module_base base;
    /**
     * Tick time */
    int ticktime;
    /** 
     * Default delay */
    int delay;
    /**
     * The list of antibounce objects */
    struct abounce *list;
};

void abounce_update(EVAL_CALL_PAR);

struct abounce_module* module_get_struct(struct module_base *base){
    return (struct abounce_module*) base;
}

/**
 * Create a antibounce object */
struct abounce *abounce_create(struct abounce_module *module, const char **attr)
{
    const char *name = get_attr_str_ptr(attr, "name");
    struct abounce *new = malloc(sizeof(*new));
    assert(new);

    new->delay = get_attr_int(attr, "delay", module->delay);
    new->copytime = get_attr_int(attr, "copytime", 1);
    new->input = evalue_create(&module->base, name,  "input" , 
			       get_attr_str_ptr(attr, "event"), DEFAULT_FEVAL);

    evalue_callback_set(new->input, abounce_update, new);
    
    new->output = evalue_create_attr(&module->base, NULL, attr);

    new->updated = 0;

    return new;
}


/**
 * Add abounce object to a list */
struct abounce *abounce_lst_add(struct abounce *list, struct abounce *new)
{
    struct abounce *ptr = list;

    if(!list){
        return new;
    }
	
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;
    return list;
}


/**
 * Delete a abounce object */
void abounce_delete(struct abounce *item)
{
	if(!item){
	    return ;
	}

	free(item);
}

/**
 * Mark as updated */
void abounce_update(EVAL_CALL_PAR)
{
    struct abounce *item = evalue->userdata;

    item->updated = 1;
}

/**
 * Run update */
void abounce_run(struct abounce *item)
{
    if(!item->updated)
	return;

    if((evalue_elapsed(item->input, NULL)*1000)> item->delay){
	struct uni_data *data = evalue_getdata(item->input);
	struct timeval *now = NULL;
	if(item->copytime)
	    now = &item->input->time;
	evalue_setdata(item->output, data, now);
	item->updated = 0;
    }
}





int  start_abounce(XML_START_PAR)
{
    struct module_base* base = ele->parent->data;
    struct abounce_module *this = module_get_struct(base);
    
    struct abounce *abounce = abounce_create(this, attr);

    if(!abounce){
	PRINT_ERR("Error: Could not create antibounce");
	return -1;
    }
    
    this->list = abounce_lst_add(this->list, abounce);
    
    
    return 0;
}



struct xml_tag module_tags[] = {
    { "chatrem" , "module" , start_abounce, NULL, NULL},
    { "" , ""  , NULL, NULL, NULL }
};
    




int module_init(struct module_base *base, const char **attr)
{
    struct abounce_module* this = module_get_struct(base);
    
    this->list = NULL;

    this->delay    = get_attr_int(attr, "delay", 2000);
    this->ticktime = get_attr_int(attr, "ticktime", 100);
    
    return 0;
}


void module_deinit(struct module_base *base)
{
    struct abounce_module* this = module_get_struct(base);
    abounce_delete(this->list);
    return;
}

/**
 *  LED panel update loop
 * @memberof ledpanel_object
 * @private
 */
void* module_loop(void *parm)
{
    struct abounce_module *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;
	
    PRINT_MVB(&this->base, "loop started (base %p)",  base);
    
    base->run = 1;
    while(base->run){ 
	struct abounce *ptr = this->list;

	while(ptr){
	    abounce_run(ptr);
	    ptr = ptr->next;
	}
	usleep(this->ticktime*1000);
    }

    PRINT_MVB(&this->base, "loop ended (base %p)", base);

    return NULL;

}


struct module_type module_type = {    
    .name       = "abounce",       
    .xml_tags   = module_tags,                      
    .type_struct_size = sizeof(struct abounce_module), 
};


struct module_type *module_get_type()
{
    return &module_type;
}
