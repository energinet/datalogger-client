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
 * @section calcmodxml Calculation module
 * @verbatim 
 <module type="calc" name="calc" verbose="1" flags="nolog">
    <sub a="modbus.rtind" b="modbus.rtoutd" name="tdiff" unit="°C" text="Temperaturdifference" sync="0" />
  </module>
  @endverbatim
  * @}
*/
enum mcalc_type{
    MCALC_ADD,
    MCALC_SUB,
    MCALC_MULT,
    MCALC_DIV,
};



struct mcalc{
    enum mcalc_type type;
	int dosync;
    struct evalue *a;
    struct evalue *b;
	struct module_calc *calc;
    struct evalue *result;
    struct mcalc *next;
};

struct calc_module{
    struct module_base base;
    struct mcalc *list;
};


struct calc_module* module_get_struct(struct module_base *base){
    return (struct calc_module*) base;
}

void mcalc_rcv_call(EVAL_CALL_PAR);

struct mcalc *mcalc_create(struct module_base *base, const char **attr, 
			 enum mcalc_type type)
{
    struct mcalc* new = malloc(sizeof(*new));
    const char *name = get_attr_str_ptr(attr, "name");

    new->type = type;
    new->next = NULL;

	new->dosync = get_attr_int(attr, "sync",1);
	
    new->a = evalue_create(base, name,  "a" , get_attr_str_ptr(attr, "a"), DEFAULT_FEVAL);
    
    new->b = evalue_create(base, name,  "b" , get_attr_str_ptr(attr, "b"), DEFAULT_FEVAL);

    evalue_callback_set(new->a, mcalc_rcv_call, new);
    evalue_callback_set(new->b, mcalc_rcv_call, new);
    
    new->result = evalue_create_attr(base, NULL, attr);

	new->calc = module_calc_create(get_attr_str_ptr(attr, "calc"),base->verbose);  

    if(!new->a || !new->b || !new->result)
		return NULL;

    return new;

}

struct mcalc *mcalc_add(struct mcalc *list, struct mcalc *new)
{
 struct mcalc *ptr = list;
    
    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}

void mcalc_delete(struct mcalc *calc_obj)
{
    if(!calc_obj)
		return ;
    
    mcalc_delete(calc_obj->next);
    free(calc_obj);
}



void mcalc_rcv_call(struct evalue *evalue)
{
    struct mcalc *calc_obj = evalue->userdata;
    struct module_base *base = evalue->base;
    int insync;
    float a, b, result;
    struct timeval stime;
    

    insync = evalue_insync(calc_obj->a, calc_obj->b, 
			   &a, &b, &stime);
    
    PRINT_MVB(base, "Received value %s%s %s (a%f b%f)", 
	      (evalue==calc_obj->a)?"A":"", 
	      (evalue==calc_obj->b)?"B":"",
	      (insync)?"in sync":"no sync",
	      a, b);
    
    PRINT_MVB(base, "A: %ld.%6.6ld, B: %ld.%6.6ld",  
			  calc_obj->a->time.tv_sec, calc_obj->a->time.tv_usec,  
			  calc_obj->b->time.tv_sec, calc_obj->b->time.tv_usec); 
    
	if(!calc_obj->dosync)
		insync = 1;

    if(insync){
		switch(calc_obj->type){
		  case MCALC_ADD:
			result = a + b;
			break;
		  case MCALC_SUB:
			result = a - b;
			break; 
		  case MCALC_MULT:
			result = a * b;
			break;
		  case MCALC_DIV:
			if(b)
				result = a / b;
			else
				insync = 0;
			break;
		  default:
			result = 0;
			break;
	    }
    }
	
    if(insync){
		result = module_calc_calc(calc_obj->calc, result);
		PRINT_MVB(base, "Result %f (time %ld.%6.6ld", result, stime.tv_sec, stime.tv_usec);
		evalue_setft(calc_obj->result, result, &stime);
    } 

    
}



int start_add(XML_START_PAR)
{
	struct module_base* base = ele->parent->data;
    struct calc_module *this = module_get_struct(base);
    struct mcalc *new = mcalc_create(base,attr, MCALC_ADD);
    this->list = mcalc_add(this->list, new);

    if(!new){
	return -1;
    }

    return 0;
    
}
int start_sub(XML_START_PAR)
{
	struct module_base* base = ele->parent->data;
    struct calc_module *this = module_get_struct(base);
    struct mcalc *new = mcalc_create(base,attr, MCALC_SUB);
    this->list = mcalc_add(this->list, new);

    if(!new){
	return -1;
    }
	
    return 0;
    
}
int start_mult(XML_START_PAR)
{
	struct module_base* base = ele->parent->data;
    struct calc_module *this = module_get_struct(base);
    struct mcalc *new = mcalc_create(base,attr, MCALC_MULT);
    this->list = mcalc_add(this->list, new);

    if(!new){
	return -1;
    }
	
    return 0;
    
}
int start_div(XML_START_PAR)
{
	struct module_base* base = ele->parent->data;
    struct calc_module *this = module_get_struct(base);
    struct mcalc *new = mcalc_create(base,attr, MCALC_DIV);
    this->list = mcalc_add(this->list, new);

    if(!new){
		return -1;
    }
	
    return 0;
    
}


struct xml_tag module_tags[] = {
    { "add"  , "module" , start_add, NULL, NULL},
    { "sub"  , "module" , start_sub, NULL, NULL},
    { "mult" , "module" , start_mult, NULL, NULL},
    { "div"  , "module" , start_div, NULL, NULL},
    { "" , ""  , NULL, NULL, NULL }
};
    

struct module_type module_type = {    
    .name       = "calc",       
    .xml_tags   = module_tags,                      
    .type_struct_size = sizeof(struct calc_module), 
};


struct module_type *module_get_type()
{
    return &module_type;
}

