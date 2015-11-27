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
#include <termios.h>

#include <syslog.h>

#include "kamcom.h"


#define DEFAULT_BAUD (1200)
#define DEFAULT_ADDR (0x3f)
#define DEFAULT_INTERVAL (30)
#define MODULE_NAME "kamcom"


/**
 * @addtogroup module_xml
 * @{
 * @section kamcom Module for reading Kamstrup meters
 * <b>Typename: "kamcom" </b>\n
 * <b> Attributes: </b>
 * - <b> path:</b> Path to the usb tty device\n
 * - <b> baud:</b> Baud rate/communication speed (defailt: 1200)
 * - <b> adr:</b> Device address
 * <b> Tags: </b>
 * <ul>
 * <li><b>get:</b> 
 * - <b> reg:</b> Register to read
 * </ul>
 @verbatim 
 <module type="kamcom" name="meter" path="/dev/ttyS1" adr="0x3f" verbose="0 ">
     <reg 
 </module>
@endverbatim
 @}
*/

enum{
    FAULT_INIT=-1,
    FAULT_NONE=0,
    FAULT_REGLOST,
    FAULT_FATAL,
    FAULT_BUS,
};


struct kamreg{
    unsigned short reg;
    int interval;
    struct event_type *event_type;
    int fault_state;
    struct module_calc *calc;

    struct kamreg *next;
};


struct kamcom_module {
    struct module_base base;
    char *path;
    unsigned short addr;
    int baud_tx;
    int baud_rx;
    int interval;
    struct kamcom *kamcom;
    struct kamreg *regs;
    struct event_type *fault_event;
    struct module_tick *tick;

    int fault_state;
};


struct kamcom_module* module_get_struct(struct module_base *base){
    return (struct kamcom_module*) base;
}



/**
 * Read event handler (public)
 */
struct module_event *kamreg_eread(struct event_type *event)
{
     struct kamreg *reg = (struct kamreg *)event->objdata;
     struct kamcom_module* this = module_get_struct(event->base);
     struct uni_data *data= NULL;
     struct kamcom_cmd *cmd;
	 int ret;

     cmd = kamcom_cmd_create_getreg(this->addr, reg->reg);

     kamcom_trx(this->kamcom, cmd);
     float value = 0;
	 int ivalue = 0;

	 //If not the serial number return regular float
	 if (reg->reg != 0x03e9){
     	if ((ret = kamcom_cmd_rd_regf(cmd, reg->reg, &value, NULL)) == 0){
	 		data = uni_data_create_float(value);
		}
	 } else { //Return the serial number as integer
     	if ((ret = kamcom_cmd_rd_regi(cmd, reg->reg, &ivalue, NULL)) == 0){
			if (reg->next && reg->reg == reg->next->reg) { //First (low) number
				ivalue = (ivalue & 0xffff);
				data = uni_data_create_int(ivalue);
			} else {
				ivalue = ((ivalue >> 16) & 0xffff);
				data = uni_data_create_int(ivalue);
			}
		}
     }
	 
	 if (ret != 0) {
		 fprintf(stderr, "Read error\n");
		 data = uni_data_create_text("error");
	 }

     kamcom_cmd_delete(cmd);

     return module_event_create(event, data,  NULL);
}


struct kamreg *kamreg_create(struct module_base *base, const char **attr){
    struct kamreg *new = malloc(sizeof(struct kamreg));
	struct kamreg *new2;
    struct kamcom_module* com = module_get_struct(base);

	char *name;
	char *hname;

    assert(new);

    new->reg = get_attr_int(attr, "reg", 0);

	//If the register is not the serial number, create regular event
	if (new->reg != 0x3e9) {
		new->interval =  get_attr_int(attr, "interval", com->interval);
		new->fault_state = FAULT_INIT;
		new->event_type = event_type_create_attr(base, (void*)new, attr); // create 
		new->event_type->read =  kamreg_eread;
		base->event_types = event_type_add(base->event_types, new->event_type); // make it avaliable

		new->calc = module_calc_create(get_attr_str_ptr(attr, "calc"),base->verbose);
		
		new->next =  NULL;
	} else {
		//In the case of the serial number, the result is split into two events.
    	new2 = malloc(sizeof(struct kamreg));
		new2->reg = 0x3e9;

		new->next = new2;
		new2->next = NULL;

		new->interval =  get_attr_int(attr, "interval", com->interval);
		new2->interval = new->interval;

		new->fault_state = FAULT_INIT;
		new2->fault_state = FAULT_INIT;

		new->event_type = event_type_create_attr(base, (void*)new, attr);
		new2->event_type = event_type_create_copy(base, (void*)new2, new->event_type->flags, new->event_type);

		name = new->event_type->name;
		hname = new->event_type->hname;

		free(new2->event_type->name);
		free(new2->event_type->hname);

		new->event_type->name = malloc(strlen(name)+2);
		new2->event_type->name = malloc(strlen(name)+2);
		sprintf(new->event_type->name, "%sl", name);
		sprintf(new2->event_type->name, "%sh", name);

		new->event_type->hname = malloc(strlen(hname)+3);
		new2->event_type->hname = malloc(strlen(hname)+3);
		sprintf(new->event_type->hname, "%s_L", hname);
		sprintf(new2->event_type->hname, "%s_H", hname);

		free(name);
		free(hname);

		new->event_type->read = kamreg_eread;
		new2->event_type->read = kamreg_eread;

		base->event_types = event_type_add(base->event_types, new->event_type);
		base->event_types = event_type_add(base->event_types, new2->event_type);
		
		//Not possible to use calc on serial
		new->calc = NULL;
		new2->calc = NULL;

	}

	PRINT_MVB(base, "kamreg read %X, interval %d sec", new->reg, new->interval);

    return new;
}

struct kamreg *kamreg_lst_add(struct kamreg *list, struct kamreg *new)
{
    struct kamreg *ptr = list;
    
    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;
    
    return list;

}


int kamreg_read(struct kamcom_module *module, struct kamreg *reg, struct timeval *time)
{      
    float value = 0;
	int ivalue = 0;
    float fvalue = 0;
    struct kamcom_cmd *cmd;

    PRINT_MVB(&module->base, "kamreg read (%X)", reg->reg);

	
    cmd = kamcom_cmd_create_getreg(module->addr, reg->reg);
    if(kamcom_trx(module->kamcom, cmd)<0){
	reg->fault_state = FAULT_FATAL;
	kamcom_cmd_delete(cmd);
	return -1;
    }
	
	//If not the serial number return float
	if (reg->reg != 0x03e9){
		if(kamcom_cmd_rd_regf(cmd, reg->reg, &value, NULL)){
			reg->fault_state = FAULT_REGLOST;
		} else {
			reg->fault_state = FAULT_NONE;
		}
	} else {
		if(kamcom_cmd_rd_regi(cmd, reg->reg, &ivalue, NULL)){
			reg->fault_state = FAULT_REGLOST;
		} else {
			reg->fault_state = FAULT_NONE;
		}
	}
    kamcom_cmd_delete(cmd);

	//If not the serial number return float
	if (reg->reg != 0x03e9){
		PRINT_MVB(&module->base, "kamreg read %f", value);
	} else {
		PRINT_MVB(&module->base, "kamreg read %d", ivalue);
	}

    if(reg->fault_state != FAULT_NONE)
		return -1;

	struct uni_data *data;

	//If not the serial number return float
	if (reg->reg != 0x03e9){
		fvalue = module_calc_calc(reg->calc, value);
	} else {
		if (reg->next && reg->reg == reg->next->reg) { //First (low) number
			fvalue = (float) (ivalue & 0xffff);
		} else {
			fvalue = (float) ((ivalue >> 16) & 0xffff);
		}
	}
	data = uni_data_create_float(fvalue);
    module_base_send_event(module_event_create(reg->event_type, data, time));

    return 0;       
}


int xml_start_reg(XML_START_PAR)
{
    struct module_base* base = ele->parent->data;
    struct kamcom_module *this = module_get_struct(base);
    struct kamreg *new = kamreg_create(base,attr);	

    if(!new){
	return -1;
    }
    
    this->regs = kamreg_lst_add(this->regs, new);
		
    return 0;
}



struct xml_tag module_tags[] = {
    { "reg" , "module" , xml_start_reg, NULL, NULL},
    { "" , ""  , NULL, NULL, NULL }
};  


int kamcom_module_fault(struct kamcom_module *this, struct timeval *now)
{
    int new_state = FAULT_NONE;
    int count = 0;
    int fault_cnt = 0;



    struct kamreg   *ptr = this->regs;

    while(ptr){
	count++;
	if(ptr->fault_state > FAULT_NONE)
	    fault_cnt++;

	if(ptr->fault_state > new_state)
	    new_state = ptr->fault_state;

	ptr = ptr->next;
    }

    if(fault_cnt)
	PRINT_MVB(&this->base, "fault %d/%d", fault_cnt, count);

    if(fault_cnt == count)
	new_state = FAULT_FATAL;

    if(new_state != this->fault_state){
	PRINT_MVB(&this->base, "fault state changed %d --> %d", this->fault_state ,new_state);
	this->fault_state = new_state;
	struct uni_data *data = uni_data_create_int(new_state);
	module_base_send_event(module_event_create(this->fault_event, data, now));
    }

    return new_state;
}


void* module_loop(void *parm)
{
    struct kamcom_module *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;

    int retval;
    struct timeval tv;

    base->run = 1;
    
    PRINT_MVB(base, "Starting loop");
    

    while(base->run){ 
        
        struct kamreg *ptr = this->regs;
	
		retval = module_tick_wait(this->tick, &tv);

		if(retval < 0){
			PRINT_ERR( "module tick error (%d)", retval);
			break;
		} 
		
		if(retval>0){
			PRINT_MVB(base, "module tick rollover (%d)", retval);
		} 

        while(ptr){

            if((tv.tv_sec%ptr->interval)==0){
	
				kamreg_read(this, ptr, &tv);
            }

            ptr = ptr->next;
        }

		kamcom_module_fault(this, &tv);
    }
    
    PRINT_MVB(base, "loop returned");
    
    return NULL;

}





int module_init(struct module_base *base, const char **attr)
{
    struct  kamcom_module *this = module_get_struct(base);
    const char *path = get_attr_str_ptr(attr, "path");

    if(!path){
	PRINT_ERR("Path must be defined in %s of type "MODULE_NAME, 
		  get_attr_str_ptr(attr, "name"));
	return -1;
    }
	
    this->path = strdup(path);

    PRINT_MVB(base, "init path: %s", this->path);

 
    
    /* openlog("kamcomutil", LOG_PID|LOG_PERROR, LOG_USER); */
    /* setlogmask(LOG_UPTO(LOG_DEBUG)); */
    


    int baud =  get_attr_int(attr, "baud", DEFAULT_BAUD);

    this->baud_tx =  get_attr_int(attr, "baud_tx", baud);
    this->baud_rx =  get_attr_int(attr, "baud_rx", baud);

    this->kamcom = kamcom_create(this->path, this->baud_tx, this->baud_rx);

    this->addr = get_attr_int(attr, "addr", DEFAULT_ADDR);


    this->fault_event = event_type_create(&this->base, NULL,"fault", 
					  "%", 
					  "Kamcom Fault Event", DEFAULT_FFEVT);

    base->event_types = event_type_add(base->event_types, this->fault_event);

    this->fault_state = FAULT_INIT;

    PRINT_MVB(base, "init TICK: %f", get_attr_float(attr, "tick", 1));

    this->tick = module_tick_create(base->tick_master,  base, get_attr_float(attr, "tick", 1), TICK_FLAG_SECALGN1|TICK_FLAG_SKIP);
    
    this->interval = get_attr_int(attr, "interval", DEFAULT_INTERVAL);

    return 0;
};





void module_deinit(struct module_base *module)
{
    struct kamcom_module* this = module_get_struct(module);

    kamcom_delete(this->kamcom);

    free(this->path);

    return;
}



struct module_type module_type = {    
    .name       = MODULE_NAME,
    .xml_tags   = module_tags,                      
    .type_struct_size = sizeof(struct kamcom_module), 
};


struct module_type *module_get_type()
{
    return &module_type;
}



