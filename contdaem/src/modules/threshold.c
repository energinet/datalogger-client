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
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <pthread.h> 


/**
 * @ingroup modules 
 * @{
 */

/**
 * @defgroup modules_threshold Module for handeling thresholds 
 * @{
 *  
 * Module for handeling threshold.\n
 * <b>Type name: threshold </b>\n
 * <b>Attributes:</b>\n
 * - \b config: The configuration name @ref sgconf and @ref dinconf\n
 * .
 * <b> Tags: </b>
 */


enum thr_omode{ 
    THR_ONCHAN, /*!< Output event on change */
    THR_ALWAYS, /*!< Output event on received event */
    THR_INTERVAL, /*!< Output event on interval */
}; 

const char *throtyps[] = { "change", "always", "interval", NULL};

enum thr_state{
	THR_STATE_NONE = -1,
	THR_STATE_MIN  =  0,
	THR_STATE_MAX  =  1,
	THR_STATE_IN   =  2,
	THR_STATE_OUT  =  3,
};

const char *throstats[] = { "min", "max", "in", "out" , NULL};

enum thr_mode{ 
    THR_MODE_THRH, /*!< Threshold, over = vmax, under vmin */
    THR_MODE_BOUND, /*!< Boundary, in = vin(vmin) , out = vout (vmax)*/
}; 

const char *thromode[] = { "threshold", "boundary", NULL};


struct thrhsmod_object;
/**
 * Threshold object struct 
 */
struct thrhld {
	enum thr_mode mode;
	enum thr_state state;
	/**
	 * Input value for comparison */
	struct evalue *value_in;
	/**
	 * Output evalue */
	struct evalue *value_out;
	/**
	 * Minimum or inbound value */
	struct evalue *min_out;
	/**
	 * Maximum or outbound value */
	struct evalue *max_out;
	/**
	 * Curent value either max_out or min_out */
	struct evalue *cur_out;


    struct evalue *min_thr;
    struct evalue *max_thr;
    struct evalue *hysteresis;
    enum thr_omode omode;
    int ointerval;
    struct event_type *event_type;
    struct thrhld *next;
	struct thrhsmod_object *thrhsmod;
};


/**
 * Threshold module object
 */
struct thrhsmod_object{
    /**
     * Module base*/
    struct module_base base;
    /**
     * Threshold list */
    struct thrhld  *thrhs;
    /**
     * Output mode */
    enum thr_omode omode;
    /**
     * Output interval
     */
    int ointerval;
    

};


struct thrhsmod_object* module_get_struct(struct module_base *module){
    return (struct thrhsmod_object*) module;
}

void thrhld_value_callback(EVAL_CALL_PAR);
void thrhld_thrcond_callback(EVAL_CALL_PAR);
/**
 * Create threshold 
 */
struct thrhld * thrhld_create(struct thrhsmod_object* module, const char **attr,enum thr_mode mode )
{

     struct thrhld *new =  malloc(sizeof(*new));
     const char *name = get_attr_str_ptr(attr, "name");
     const char *omode = get_attr_str_ptr(attr, "omode");
     const char *hysteresis = get_attr_str_ptr(attr, "hyst");
	 const char *vmin = get_attr_str_ptr(attr, "vmin");
	 const char *vmax = get_attr_str_ptr(attr, "vmax");

     assert(new);
     memset(new, 0 , sizeof(*new));
     
     PRINT_MVB(&module->base, "111 %s", name);
     if(get_attr_str_ptr(attr, "thr")){
		 new->max_thr = evalue_create(&module->base, name,  "thr" , get_attr_str_ptr(attr, "thr"), DEFAULT_FEVAL);
		 evalue_callback_set(new->max_thr, thrhld_thrcond_callback, new);
		 new->min_thr = new->max_thr;
     } else {
		 new->max_thr = evalue_create(&module->base, name,  "max" , get_attr_str_ptr(attr, "max"), DEFAULT_FEVAL);
		 evalue_callback_set(new->max_thr, thrhld_thrcond_callback, new);
		 new->min_thr = evalue_create(&module->base, name,  "min" , get_attr_str_ptr(attr, "min"), DEFAULT_FEVAL);
		 evalue_callback_set(new->min_thr, thrhld_thrcond_callback, new);
     }

     if(hysteresis){
		 new->hysteresis = evalue_create(&module->base, name,  "hyst" , hysteresis, DEFAULT_FEVAL);
		 evalue_callback_set(new->hysteresis, thrhld_thrcond_callback, new);
	 }

	 /* Set vmin and vmax or vin or vout */

	 if(!vmin){
		 vmin = get_attr_str_ptr(attr, "vin");
	 }

	 if(!vmax){
		 vmax = get_attr_str_ptr(attr, "vout");
	 }

	 new->min_out = evalue_create(&module->base, name,  "vmin" , vmin, DEFAULT_FEVAL);
	 evalue_callback_set(new->min_out, thrhld_value_callback, new);

	 new->max_out = evalue_create(&module->base, name,  "vmax" , vmax, DEFAULT_FEVAL);
	 evalue_callback_set(new->max_out, thrhld_value_callback, new);

	 /* set default state */

	 new->state = modutil_get_listitem(get_attr_str_ptr(attr, "default") ,THR_STATE_NONE , throstats);

	 new->value_in = evalue_create(&module->base, name,  "in" , get_attr_str_ptr(attr, "event"), DEFAULT_FEVAL);
     evalue_callback_set(new->value_in, thrhld_thrcond_callback, new);

     new->ointerval = get_attr_int(attr, "interval", module->ointerval);

     if(new->ointerval)
		 new->omode = THR_INTERVAL;
     else
		 new->omode =  module->omode;

     new->omode = modutil_get_listitem(omode, new->omode, throtyps);

	 new->thrhsmod = module;

	 new->value_out = evalue_create_attr(&module->base, NULL, attr);
	 
	 new->mode = mode;
	 
	 PRINT_MVB(&module->base, "type: %s, max %f, min %f, state %s, %s %f, %s %f %s",thromode[new->mode],
			   evalue_getf(new->max_thr),evalue_getf(new->min_thr), (new->state>=0)?throstats[new->state]:"none",
			   (new->mode==THR_MODE_THRH)?"vmax":"vout", evalue_getf(new->max_out),
			   (new->mode==THR_MODE_THRH)?"vmin":"vin", evalue_getf(new->min_out), throtyps[new->omode]
		 );

     return new;

}



/**
 * Add modbus action to list
 */
struct thrhld * thrhld_add(struct thrhld* first, struct thrhld*new)
{
    struct thrhld *ptr = first;

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




/**
 * Delete modbus action and list
 */
void  thrhld_delete(struct thrhld *thrh)
{
    
    if(!thrh)
	return;

    thrhld_delete(thrh->next);

	evalue_delete(thrh->value_in);
	evalue_delete(thrh->min_out);
	evalue_delete(thrh->max_out);

	if(thrh->min_thr != thrh->max_thr)
		evalue_delete(thrh->min_thr);

	evalue_delete(thrh->max_thr);
	evalue_delete(thrh->hysteresis);

    free(thrh);

}





/**************************************************/
/* Handlers                                       */
/**************************************************/


/**
 * Update threshold state 
 */


enum thr_state thrhld_state_upd_thrs(struct thrhld *thrh, float value)
{
	enum thr_state state = thrh->state;
	float hyst = 0;
	
	if(thrh->hysteresis)
		hyst = evalue_getf(thrh->hysteresis);

    if(value >= (evalue_getf(thrh->max_thr)+hyst))
		state = THR_STATE_MAX;

    if(value <= (evalue_getf(thrh->min_thr)-hyst))
		state = THR_STATE_MIN;
	
	return state;
}

enum thr_state thrhld_state_upd_bound(struct thrhld *thrh, float value)
{
	enum thr_state state = THR_STATE_IN;

    if((value < evalue_getf(thrh->min_thr))||
	   (value > evalue_getf(thrh->max_thr)))
		state = THR_STATE_OUT;

	return state;

}


int thrhld_state_upd(struct thrhld *thrh, float value)
{
    enum thr_state  state = thrh->state;

	switch(thrh->mode){
	  case THR_MODE_THRH:
		state = thrhld_state_upd_thrs(thrh, value);
		break;
	  case THR_MODE_BOUND:
		state = thrhld_state_upd_bound(thrh, value);
		break;
	}

    if(state == thrh->state)
		return 0;

    thrh->state = state;	

    return 1;
    
}

int thrhld_state_intupd(struct thrhld *thrh, struct timeval *now)
{
    switch(thrh->omode){
      case THR_INTERVAL:
		if(now->tv_sec%thrh->ointerval)
			return 0;
		else 
			return 1;
      case THR_ALWAYS:
		return 1;
      case THR_ONCHAN:
      default:
		return 0;
    }


}


int thrhld_update(struct thrhld *thrh, struct timeval *tv)
{

	float value = evalue_getf(thrh->value_in);

	if(!evalue_isvalid(thrh->value_in)){
	  PRINT_MVB(&thrh->thrhsmod->base,"Value is invalid : %f , \n",value  ); 
	  return 0;
	}

	if(thrhld_state_upd(thrh, value)||thrhld_state_intupd(thrh, tv)){
		switch(thrh->state){
		  case THR_STATE_MAX:
		  case THR_STATE_OUT:
			thrh->cur_out = thrh->max_out;
			break;
		  case THR_STATE_MIN:
		  case THR_STATE_IN:
			thrh->cur_out = thrh->min_out;
			break;
		  default:
			PRINT_ERR("Unknown state : %d , \n",thrh->state ); 
			thrh->cur_out = NULL;
			break;
		}

		if(thrh->cur_out){
			struct uni_data *outdata = evalue_getdata(thrh->cur_out);
			
			if(outdata){
				PRINT_MVB(&thrh->thrhsmod->base,"Sending value state change : %d , \n",thrh->state ); 
				evalue_setdata(thrh->value_out, outdata, tv);
			}
		}
    }
	return 0;
}


void thrhld_value_callback(EVAL_CALL_PAR)
{
	struct thrhld *thrh = (struct thrhld *)evalue->userdata;
	
	if(evalue != thrh->cur_out)
		return;

	struct uni_data *outdata = evalue_getdata(thrh->cur_out);

	if(!outdata)
		return;

	PRINT_MVB(&thrh->thrhsmod->base,"Sending value update : %d , \n",thrh->state ); 
	evalue_setdata(thrh->value_out, outdata,  &evalue->time);

}


void thrhld_thrcond_callback(EVAL_CALL_PAR)
{
	struct thrhld *thrh = (struct thrhld *)evalue->userdata;

	thrhld_update(thrh, &evalue->time);
	
}

/**
 * Read event handler
 */

struct event_handler handlers[] = {{.name = ""}};

int start_boundary(XML_START_PAR)
{
    struct modules *modules = (struct modules*)data;
    struct module_base* base = ele->parent->data;
	struct thrhsmod_object* this = ele->parent->data;
    PRINT_MVB(base, "Opening threshold %s", get_attr_str_ptr(attr, "event") );

	struct thrhld *new = thrhld_create(this, attr, THR_MODE_BOUND);
	this->thrhs = thrhld_add(this->thrhs, new);

	return 0;
    
}


int start_threshold(XML_START_PAR)
{
    struct modules *modules = (struct modules*)data;
    struct module_base* base = ele->parent->data;
	struct thrhsmod_object* this = ele->parent->data;
    PRINT_MVB(base, "Opening threshold %s", get_attr_str_ptr(attr, "event") );

	struct thrhld *new = thrhld_create(this, attr, THR_MODE_THRH);
	this->thrhs = thrhld_add(this->thrhs, new);

	return 0;
    
}


struct xml_tag module_tags[] = {                         \
    { "hysteresis" , "module" , start_threshold, NULL, NULL},   \
    { "threshold" , "module" , start_threshold, NULL, NULL},		 \
	{ "bound" , "module" , start_boundary, NULL, NULL},		 \
    { "" , "" , NULL, NULL, NULL }                       \
};




int module_init(struct module_base *base, const char **attr)
{
   struct thrhsmod_object *this = module_get_struct(base);
   const char *omode = get_attr_str_ptr(attr, "omode");

   this->ointerval =  get_attr_int(attr, "interval", 0);
   
   this->omode = THR_ONCHAN;
   if(this->ointerval)
       this->omode = THR_INTERVAL;
   
   this->omode = modutil_get_listitem(omode, this->omode, throtyps);

   base->verbose = 1;


   

}


/* struct event_handler *module_subscribe(struct module_base *module, struct module_base *source,  */
/* 									   struct event_type *event_type, const char **attr) */
/* { */
/*     struct thrhsmod_object *this = module_get_struct(module); */
/*     struct thrhld *new = thrhld_create(this, attr); */
/*     struct event_handler *event_handler = NULL; */
/*     const char *name = get_attr_str_ptr(attr, "name"); */
/*     const char *unit = get_attr_str_ptr(attr, "unit"); */
/*     char ename[64]; */
/*     if(strcmp(event_type->name, "fault")==0) */
/* 	return NULL; */

/*     sprintf(ename, "%s.%s", source->name, event_type->name); */
      
/*     if(!name) */
/* 	name = event_type->name; */

/*     if(!unit) */
/* 	unit = ""; */

    
/*     new->event_type =  event_type_create(module, (void*)new,   */
/* 			     name, unit,  */
/* 			     get_attr_str_ptr(attr, "text"),  */
/* 			     event_type_get_flags(get_attr_str_ptr(attr, "flags"), module->flags));  */

/*     new->event_type->read =  modbus_eread; */

/*     event_handler = event_handler_create(event_type->name, handler_rcv ,&this->base, (void*)new); */
/*     this->base.event_handlers = event_handler_list_add(this->base.event_handlers, event_handler); */
/*     this->base.event_types = event_type_add(this->base.event_types, new->event_type); // make it avaliable */

/*     this->thrhs = thrhld_add(this->thrhs, new); */
/*     PRINT_MVB(module, "Thershold: %s, omode '%s', %d ", ename, throtyps[new->omode], */
/* 	      new->ointerval ); */
/*     return event_handler; */
/* } */



struct module_type module_type = {                  \
    .name       = "threshold",                       \
    .xml_tags   = module_tags,                      \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct thrhsmod_object), \
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
