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
 * @addtogroup module_xml
 * @{
 * @section sec_control_module_xml PID controller module 
 *  
 * Module handeling PID control.\n
 * <b>Type name: control </b>\n
 * <b>Attributes:</b>\n
 * - Standard attributes..
 * .
 * <b> Tags: </b>
 * <ul>
 * <li><b>pid:</b> Create a PID controller\n  
 *   - \b name: ename of the PID controller
 *   - \b input: Input to the controller
 *   - \b p: Proportional gain \f$K_p\f$
 *   - \b i: Integral gain \f$K_i\f$
 *   - \b d: Derivative gain \f$K_d\f$
 *   - \b setpoint: Setpoint for the controller 
 *   - \b omax: Maximal output
 *   - \b omin: Maximal input
 *   - \b enabled: enabeling (true) or disabeling (false) the controller 
@verbatim<pid p="30" i="15" d="7.5" setpoint="cmdvars.temp#100" name="pid1" 
  input="termo.t8" omax="1000" omin="0" enabled="false" />@endverbatim
  \f[
  u(t)= K_pe(t) + \int_{0}^t K_i e(\tau) d\tau + K_d \frac{d}{dt} e(t)
  \f]
 * @verbatim 
  <module type="control" name="control" verbose="3">
    <pid p="2" i="2" d="1" setpoint="0" name="pid1" event="adc.v1" omin="0" omax="4095" enabled="0" />
  </module> 
  @endverbatim
  * @}
*/

/**
 * @ingroup modules 
 * @{
 */

/**
 * @defgroup modules_control Module for controllers
 * @{
 */




/**
 * Threshold object struct 
 */
struct pidcont {
    /**
     * Proportional gain */
    struct evalue *cont_p;
    /**
     * Integral gain */
    struct evalue *cont_i;
    /**
     * Derivative gain */
    struct evalue *cont_d;
    /**
     * Controller setpoint */
    struct evalue *setpoint;
    /**
     * Controller output 
     * Can be read and written, but the otput will be overloaded with the controller output 
     * if the controller is enabled */
    struct evalue *output;
    /**
     * Maximal output value
     * @note The integral error will not be updated if output is at omax */
    struct evalue *omax;
    /**
     * Minimal output value
     * @note The integral error will not be updated if output is at omin */
    struct evalue *omin;
    /**
     * Integral error value (r/w)*/
    struct evalue *intg_err;
    /**
     * Dead time */
    struct evalue *deadtime;
    /**
     * Enabled value */
    struct evalue *enabled ;
    /**
     * Last recoeded error value */
    float last_seterr;
    /**
     * Time of last received input value */
    struct timeval t_last;
    /**
     * Next PID controller object */
    struct pidcont *next;
};

/**
 * controller module object
 */
struct control_object{
    /**
     * Module base*/
    struct module_base base;
    /**
     * PID controller list */
    struct pidcont  *pids;
};


struct control_object* module_get_struct(struct module_base *base){
    return (struct control_object*) base;
}

/**
 * Create a PID controller object 
 * @param module the controller module
 * @param attr the XML attributes
 * @return a PID controller object og NULL if error
 */
struct pidcont *pidcont_create(struct control_object* module, const char **attr)
{
    struct pidcont *new =  malloc(sizeof(*new));
    const char *name = get_attr_str_ptr(attr, "name");
    const char *intg_err = get_attr_str_ptr(attr, "intg_err");
    const char *enabled  =  get_attr_str_ptr(attr, "enabled");
    const char *deadtime  =  get_attr_str_ptr(attr, "deadtime");
    
    assert(new);

    memset(new, 0, sizeof(*new));

    new->cont_p = evalue_create(&module->base, name,  "p" , get_attr_str_ptr(attr, "p"), DEFAULT_FEVAL);
    new->cont_i = evalue_create(&module->base, name,  "i" , get_attr_str_ptr(attr, "i"), DEFAULT_FEVAL);
    new->cont_d = evalue_create(&module->base, name,  "d" , get_attr_str_ptr(attr, "d"), DEFAULT_FEVAL);
    new->omax = evalue_create(&module->base, name,  "omax" , get_attr_str_ptr(attr, "omax"), DEFAULT_FEVAL);
    new->omin = evalue_create(&module->base, name,  "omin" , get_attr_str_ptr(attr, "omin"), DEFAULT_FEVAL);
    new->setpoint = evalue_create(&module->base, name,  "setpoint" , get_attr_str_ptr(attr, "setpoint"), DEFAULT_FEVAL);
    new->output = evalue_create(&module->base, name, NULL  , "0", DEFAULT_FEVAL);

    
    if(!intg_err)
	intg_err = "0";
    
    new->intg_err = evalue_create(&module->base, name, "intg_err"  , intg_err , DEFAULT_FEVAL);
    
    if(!enabled)
	enabled = "1";

   new->enabled = evalue_create(&module->base, name, "enabled"  , enabled , DEFAULT_FEVAL);

   if(!deadtime)
       deadtime = "0";
   
   new->deadtime = evalue_create(&module->base, name, "deadtime"  , deadtime , DEFAULT_FEVAL);

   return new;
}

/**
 * Add a PID controller object to a list
 * @param list the first element of the list of objects 
 * @param new the new object to bed added
 * @return the new first element of the list 
 */
struct pidcont *pidcont_add(struct pidcont *list, struct pidcont *new)
{
    struct pidcont *ptr = list;

    if(!list)
        /* first module in the list */
        return new;

    /* find the last module in the list */
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}

/**
 * Delete a PID controller object 
 * @param pidcont the PID controller object 
 */
void pidcont_delete(struct pidcont *pidcont)
{
    if(!pidcont)
	return;
    
    pidcont_delete(pidcont->next);

    evalue_delete(pidcont->cont_p);
    evalue_delete(pidcont->cont_i);
    evalue_delete(pidcont->cont_d);
    evalue_delete(pidcont->setpoint);
    evalue_delete(pidcont->output);

    evalue_delete(pidcont->deadtime);
    evalue_delete(pidcont->enabled);
    evalue_delete(pidcont->omax);
    evalue_delete(pidcont->omin);
    free(pidcont);
}

/**
 * Do PID controller action 
 * @param pidcont the PID controller object 
 * @param input the input value 
 * @param t_now the time of the received input
 * @param dbglev debug level
 * @return the output value
 */
float pidcont_action(struct pidcont *pidcont, float input, struct timeval *t_now, int dbglev)
{
    float k_p = evalue_getf(pidcont->cont_p);
    float k_i = evalue_getf(pidcont->cont_i);
    float k_d = evalue_getf(pidcont->cont_d);
    float omax =  evalue_getf(pidcont->omax);
    float omin =  evalue_getf(pidcont->omin);
    float seterror = evalue_getf(pidcont->setpoint) - input;
    float t_diff = 0;
    float intg_err = evalue_getf(pidcont->intg_err);
    float deadtime = evalue_getf(pidcont->deadtime);

    if(pidcont->t_last.tv_sec)
	t_diff = ((float)modutil_timeval_diff_ms(t_now, &pidcont->t_last))/1000;

    if(t_diff && t_diff < deadtime){
	if(dbglev&2)
	    fprintf(stderr, "dead time %f < %f\n", t_diff , deadtime);
	return evalue_getf(pidcont->output);
    }
    
    float e_p = k_p * seterror;
   
    if(t_diff)
	intg_err += seterror*t_diff*k_i;

    float e_i = intg_err;
    float e_d = 0;
    if(t_diff)
	e_d = k_d * (( seterror - pidcont->last_seterr)/t_diff);
    pidcont->last_seterr = seterror;
    float output = e_p + e_i + e_d; 
   

    if(output > omax){
	output = omax;
	if(seterror < 0)
	    evalue_setf(pidcont->intg_err,intg_err);
    } else if (output < omin){
	output = omin;
	if(seterror > 0)
	    evalue_setf(pidcont->intg_err,intg_err);
    } else {
	evalue_setf(pidcont->intg_err,intg_err);
    }

    if(dbglev&2)
	fprintf(stderr, "in %2.2f time %d.%3.3lu err %5.2f tdif %4.2f pe%5.1f, ie%5.1f, de %5.1f, out:%5.2f\n", 
			input, (int)t_now->tv_sec, t_now->tv_usec/1000, seterror, t_diff , e_p, e_i , e_d,   
			output);

    memcpy(&pidcont->t_last, t_now, sizeof(struct timeval));

   

    return output;
    
}


/**
 * Handler for receiving input events 
 */
int handler_rcv_input(EVENT_HANDLER_PAR)
{
    struct control_object* this = module_get_struct(handler->base);
    struct pidcont *pidcont = (struct pidcont *)handler->objdata;
    struct uni_data *data = event->data;
    float value = uni_data_get_fvalue(data);
    float output = 0;

    PRINT_MVB(&this->base,"Received value : %f %f %f\n", value, evalue_getf(pidcont->enabled), evalue_getf(pidcont->setpoint));
   
    if(evalue_getf(pidcont->enabled)!=0){
	output = pidcont_action(pidcont, value, &event->time, handler->base->verbose);
	if(evalue_getf(pidcont->enabled)!=0)
	    evalue_setf(pidcont->output, output);
    } else {
	memset(&pidcont->t_last, 0, sizeof(struct timeval));
	output  = 0;
    }

   

    PRINT_MVB(&this->base,"Output value  : %f , \n", output  ); 

    return 0;
}



struct event_handler handlers[] = {{.name = "rcv", .function = handler_rcv_input} , {.name = ""}};


/**
 * Start PID controller tag
 */
int start_pid(XML_START_PAR)
{
    struct control_object* this = module_get_struct(ele->parent->data);
    struct modules *modules = (struct modules*)data;
    const char *event_name = get_attr_str_ptr(attr, "input");
    struct pidcont *new = pidcont_create(this, attr);
    struct event_handler *event_handler = NULL;

    if(!event_name)
	event_name = get_attr_str_ptr(attr, "event");
    
    PRINT_MVB(&this->base, "Creating pid control. Input: %s", event_name );
    
    /* create event handler */
    event_handler = event_handler_create(get_attr_str_ptr(attr, "name")
					 ,  handler_rcv_input ,&this->base, (void*)new);
    
    this->base.event_handlers = event_handler_list_add(this->base.event_handlers, event_handler);

    this->pids = pidcont_add(this->pids, new);

    return module_base_subscribe_event(&this->base, modules->list, event_name, FLAG_ALL, 
				       event_handler, attr);
    

}



struct xml_tag module_tags[] = {
    { "pid" , "module" , start_pid, NULL, NULL},
    { "" , "" , NULL, NULL, NULL }
};


/**
 * Initialice the controller module
 */
int module_init(struct module_base *base, const char **attr)
{
	return 0;
}


/**
 * Module definition struct 
 */
struct module_type module_type = {
    .name       = "control",
    .xml_tags   = module_tags,
    .handlers   = handlers ,
    .type_struct_size = sizeof(struct control_object),
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
