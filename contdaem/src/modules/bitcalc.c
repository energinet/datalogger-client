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
 * @section bitvcalcmodxml Bit calculation module
 * Module for haneling bit calculations.
 * <b>Typename: "bitcalc" </b>\n
 * <b> Tags: </b>
 * <ul>
 * <li><b>value:</b> Canculate the value from the incomming bits
 * <li><b>and:</b> And all the bits. Outputs 1 if all bits are high.
 * <li><b>or:</b> Or all the bits Outputs 1 if just one bits is high.
 * <li><b>count:</b> Count the high.
 * </ul>
 * All of the above listens to a number of bits. The bits are inserted as tags insige the above tags. 
 @verbatim 
 <value name="...">
   <bit event="..."/>
   <bit event="..."/>
 </value>
@endverbatim
 * <ul>
 * <li><b>bit:</b> Insertes a bit into the calculation. There can be a maximum of 32 bits in a calculation.
 * A value is 1 if it is > 0.
 * - <b> invert:</b> Invert the bit \n
 * </ul>
 @}
*/




const char *bcalctype_str[] = { "value", "and", "or", "count" , NULL};


enum bcalctype{
    BCALC_VALUE,
    BCALC_AND,
    BCALC_OR,
    BCALC_CNT,
};



struct bbit{
    struct evalue *bvalue;
    int invert;
    struct bbit *next;
};


struct bcalc{
    enum bcalctype type;
    struct bbit *bbits;
    int bitcount;
    struct evalue *result;
    const char *name;
    struct bcalc *next;
};

struct bcalc_module{
    struct module_base base;
    struct bcalc *list;
};


/*
  xml config
  <module type="bcalc" name="...">
    <value name="...">
      <bit event="..."/>
      <bit event="..."/>
	</value>
  </module>
 */


struct bcalc_module* module_get_struct(struct module_base *base){
    return (struct bcalc_module*) base;
}

/************************************/
/* Bit handeling                    */

void bcalc_rcv_call(struct evalue *evalue);


struct bbit *bbit_create(struct module_base *base, struct bcalc *bcalc,  const char **attr)
{
	struct bbit* new = malloc(sizeof(*new));

	char subname[32];
	
	sprintf(subname, "bit%d", bcalc->bitcount++);

	new->invert = get_attr_int(attr, "invert", /*default*/ 0);
	new->bvalue = evalue_create(base, bcalc->name,  subname , get_attr_str_ptr(attr, "event"), DEFAULT_FEVAL);

	if(!new->bvalue){
		PRINT_ERR("Error could not create evalue");
		free(new);
		return NULL;
	}

	evalue_callback_set(new->bvalue, bcalc_rcv_call, bcalc);



	return new;
}

struct bbit *bbit_add(struct bbit *list, struct bbit *new)
{
	new->next = list;

	return new;

}

void bbit_delete(struct bbit *bbit)
{
	/* ToDo: Delete evalue */

	if(!bbit)
		bbit_delete(bbit->next);
	
	free(bbit);
}


int bbit_value(struct bbit *bbit)
{
	float value = evalue_getf(bbit->bvalue);

	if(value > 0)
		return 1;
	else
		return 0;
}

struct bcalc *bcalc_create(struct module_base *base, const char *type, const char **attr )
{
    struct bcalc* new = malloc(sizeof(*new));
	
    new->type = modutil_get_listitem(type, /*default*/BCALC_VALUE, bcalctype_str);
    new->bbits = NULL;
    new->bitcount = 0;
    new->result = evalue_create_attr(base, NULL, attr);	
    
    if(!new->result){
	/*ToDo: Err msg */
	return NULL;
    }
    
    new->name = new->result->name;
    
    return new;
}

struct bcalc *bcalc_add(struct bcalc *list, struct bcalc *new )
{
	struct bcalc *ptr = list;
    
    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}

void bcalc_delete(struct bcalc *bcalc)
{
	/* ToDo: Delete evalue */

	if(!bcalc)
		bcalc_delete(bcalc->next);
	
	free(bcalc);
}




void bcalc_rcv_call(struct evalue *evalue)
{
	
	struct bcalc *calc_obj = evalue->userdata;
    struct module_base *base = evalue->base;
	struct bbit *ptr;
	unsigned long result = 0;
	struct timeval stime;

	assert(calc_obj);
	assert(base);

	PRINT_MVB(base, "bcalc_rcv_call %s", evalue->name);

	if(!calc_obj->bbits){
		PRINT_ERR("Error no bits in value");
		return;
	}

	memcpy(&stime, &evalue->time, sizeof(struct timeval));		
	
	ptr = calc_obj->bbits;

	while(ptr){
		int bit = bbit_value(ptr);

		if(ptr->invert)
		    bit = !bit;

		result = result<<1;

		if(bit)
			result |= 1;

		ptr = ptr->next;
	}

	PRINT_MVB(base, "Result %lx (time %ld.%6.6ld)", result, stime.tv_sec, stime.tv_usec);

	switch(calc_obj->type){
	case BCALC_VALUE:
	    break;
	case BCALC_AND:
	    result = (result==((1<<calc_obj->bitcount)-1));
	    break;
	case BCALC_OR:
	    result = (result>0)?1:0;
	    break;
	case BCALC_CNT:
	    result = __builtin_popcount(result);
	    break;
	default:
	    break;
	}

	evalue_setft(calc_obj->result, result, &stime);	

}

int xml_start_bit(XML_START_PAR);

struct xml_tag value_tags[] = {
    { "bit"  , "value" , xml_start_bit, NULL, NULL},
    { "bit"  , "and" , xml_start_bit, NULL, NULL},
    { "bit"  , "or" , xml_start_bit, NULL, NULL},
    { "bit"  , "count" , xml_start_bit, NULL, NULL},
    { "" , ""  , NULL, NULL, NULL }
};
  



int xml_start_value(XML_START_PAR)
{
	struct module_base* base = ele->parent->data;
	struct bcalc_module *this = module_get_struct(base);
	struct bcalc *new = bcalc_create(base,el, attr);	

	ele->ext_tags = value_tags;

	this->list = bcalc_add(this->list, new);

	if(!new){
		return -1;
    }

	ele->data = new;
	
    return 0;
}

void xml_end_value(XML_END_PAR) 
{ 
	PRINT_ERR("xml_end_value %s", ele->name);
	
	ele->data = NULL;

} 


int xml_start_bit(XML_START_PAR)
{
	struct bcalc_module *this = ele->parent->parent->data;
	struct bcalc *bcalc = ele->parent->data;
	const char *ename = get_attr_str_ptr(attr, "event");
	struct bbit *new = NULL;

	PRINT_MVB(&this->base, "xml_start_bit %p %p %p", this, bcalc, ename);

	if(!ename){
		PRINT_ERR("Please provide event for bit");
		return -1;
	}

	new = bbit_create(&this->base, bcalc, attr);

	if(!new){
		PRINT_ERR("Error could not ");
		return -1;
	}

	bcalc->bbits = bbit_add(bcalc->bbits, new);

	PRINT_MVB(&this->base, "xml_start_bit %p %p ", 	bcalc->bbits, new);	

	return 0;

}




struct xml_tag module_tags[] = {
    { "value"  , "module" , xml_start_value, NULL, xml_end_value},
    { "and"  , "module" , xml_start_value, NULL, xml_end_value},
    { "or"  , "module" , xml_start_value, NULL, xml_end_value},
    { "count"  , "module" , xml_start_value, NULL, xml_end_value},
    { "" , ""  , NULL, NULL, NULL }
};
  

struct module_type module_type = {    
    .name       = "bitcalc",       
    .xml_tags   = module_tags,                      
    .type_struct_size = sizeof(struct bcalc_module), 
};


struct module_type *module_get_type()
{
    return &module_type;
}

