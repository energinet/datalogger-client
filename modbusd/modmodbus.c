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

#include "msgpipe.h"

enum{
    FAULT_INIT=-1,
    FAULT_NONE=0,
    FAULT_MLOST,
    FAULT_UNIT,
    FAULT_BUS,
};


enum act_type {
    ACTTYPE_READ,
    ACTTYPE_FREAD,
    ACTTYPE_WRITE    
};

enum act_signaling{
    ACTSIG_ALWAYS,
    ACTSIG_CHANGE,
    ACTSIG_TOHIGH,
    ACTSIG_TOLOW,   
};


struct modbus_reg{
    int mod_id;
    enum eRegTypes   type;
    int reg;
    enum eValueTypes vtype;
};

struct modb_read{
    struct modbus_reg reg;
    struct modbus_reg r_chk;
    float last_read;
    enum act_signaling signaling;
    int invert;
    int interval_sig;
};

struct modb_write{
    struct modbus_reg dest;
    int last_set;
    int mult;
};

struct modb_fread{
    struct modbus_reg reg_pulse;
    struct modbus_reg reg_time;
    float upp;
    float last_read_pulse;
    float last_read_time;
    float acc_read_pulse;
    float acc_read_time;
    int sig_interval;
    struct timeval last_signal;
    struct timeval last_pulse;
};

struct modbus_act{
    int atype;
    int interval;
    struct module_calc *calc;
    struct event_type *event_type;
    union{
	struct modb_read  read;
	struct modb_fread fread;
	struct modb_write write;
    };
    struct timeval last_update;
    struct timeval last_read;
    int msg_cnt;
    int msg_err;
    int fault_state;
    struct modbus_act *next;
};

struct module_object{
    struct module_base base;
    int mq_id;
	int trxdelay;
    struct modbus_act  *acts;
    struct event_type *status_event;
    struct event_type *fault_event;
    int fault_state;
    struct module_tick *tick;
};

struct module_object* module_get_struct(struct module_base *module){
    return (struct module_object*) module;
}
struct module_event *modbus_eread(struct event_type *event);

// ToDo finde the bug in xmlpar compile
float get_attr_float(const char **attr, const char *id, float rep_value)
{
    int attr_no;
    float ret; 
        
    attr_no = get_attr_no(attr, id);
    
    if (attr_no < 0){
        return rep_value;
    }
    
    if(sscanf(attr[attr_no], "%f", &ret)!=1){
        printf("Attribute %s could not be red: %s\n", id,attr[attr_no] );
        return rep_value;
    }
    printf("get_attr_float %f\n", ret); 
    
    return ret;
}



enum eRegTypes modbus_get_type(const char *typestr, enum eRegTypes def){

    enum eRegTypes type = def;

    if(!typestr)
        return type;

    if(strncmp(typestr, "coil", 4) == 0) {
	type = MB_TYPE_COILS;
    } else if(strncmp(typestr, "input", 5) == 0) {
	type = MB_TYPE_INPUT;
    } else if(strncmp(typestr, "holding", 7) == 0) {
	type = MB_TYPE_HOLDING;
    } else if(strncmp(typestr, "slaveid", 7) == 0) {
	type = MB_TYPE_SLAVEID;
    } else {
	type = MB_TYPE_UNKNOWN;
    }

    return type;
    
}

enum act_signaling module_get_signaling(const char *sigstr, enum act_signaling def)
{
    enum act_signaling signal = def;

    if(!sigstr)
        return signal;
    
    if(strcmp(sigstr, "always") == 0) {
	signal = ACTSIG_ALWAYS;
    } else if(strcmp(sigstr, "change") == 0) {
	signal = ACTSIG_CHANGE;
    } else if(strcmp(sigstr, "tohigh") == 0) {
	signal = ACTSIG_TOHIGH;
    } else if(strcmp(sigstr, "tolow") == 0) {
	signal = ACTSIG_TOLOW;
    } 
    
    return signal;
     
}


int modbus_reg_fill(struct modbus_reg *reg, const char **attr)
{
    
    const char *typestr = get_attr_str_ptr(attr, "type");


    reg->type   =  modbus_get_type(typestr, MB_TYPE_INPUT);
    reg->mod_id = get_attr_int(attr, "id", 0);
    reg->reg    = get_attr_int(attr, "reg", 0);


    if(get_attr_int(attr, "long", 0)){
	reg->vtype = MV_TYPE_LONG;
    }else if(get_attr_int(attr, "unsigned", 0)){
	reg->vtype = MV_TYPE_USHORT;
    } else {
	reg->vtype = MV_TYPE_SHORT;
    }
    
    return 0;

}

/**
 * Create modbus action
 */
struct modbus_act * modbus_act_create(struct module_object* module, enum act_type atype, const char **attr)
{
     struct modbus_act *new =  malloc(sizeof(*new));
     assert(new);
     memset(new, 0 , sizeof(*new));
     struct modbus_reg *reg = NULL;
     int def_interval = 300;

     new->atype = atype;

     switch(atype){
       case ACTTYPE_READ:
	 reg = &new->read.reg;
	 new->read.signaling = module_get_signaling(get_attr_str_ptr(attr, "signaling"), ACTSIG_ALWAYS);
	 new->read.invert = get_attr_int(attr, "invert", 0);
	 new->read.interval_sig = get_attr_int(attr, "sig_interval", 0);
	 def_interval = 1;
	 break;
      case ACTTYPE_FREAD:
	reg = &new->fread.reg_pulse;
	new->fread.upp=  get_attr_float(attr, "upp", 1);
	modbus_reg_fill(&new->fread.reg_time, attr);
	new->fread.reg_time.reg = get_attr_int(attr, "reg_t", 0);
	new->fread.sig_interval = get_attr_int(attr, "sig_interval", 300);
	def_interval = 1;
	break;
      case ACTTYPE_WRITE:
	reg = &new->write.dest;
	new->write.mult = get_attr_int(attr, "mult", 0);
	break;
       default:
	 goto err_out;
     }

     if(modbus_reg_fill(reg, attr)!=0)
	 goto err_out;

     new->interval =  get_attr_int(attr, "interval", def_interval );

     new->calc = module_calc_create(get_attr_str_ptr(attr, "calc"),module->base.verbose);
				    
     new->event_type = event_type_create_attr(&module->base, (void*)new, attr); // create 
     new->event_type->read =  modbus_eread;

     PRINT_MVB(&module->base,"Created modbus action type: %d , \n", atype); 

     return new;

  err_out:

     free(new);
    
     return NULL;


}

/**
 * Add modbus action to list
 */
struct modbus_act * modbus_act_add(struct modbus_act* first, struct modbus_act *new)
{
    struct modbus_act *ptr = first;

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
void  modbus_act_delete(struct modbus_act *act)
{
    
    if(!act)
	return;

    modbus_act_delete(act->next);

    free(act);

}

/**************************************************/
/* Handlers                                       */
/**************************************************/

/**
 * Inbound event hantler 
 */
int handler_rcv(EVENT_HANDLER_PAR)
{
    struct module_object* this = module_get_struct(handler->base);
    struct modbus_act *act = (struct modbus_act*)handler->objdata;
    struct modbus_reg *dest = &act->write.dest;
    struct uni_data *data = event->data;
    float value = uni_data_get_fvalue(data);

    write_cmd(this->mq_id,  dest->mod_id, dest->type, dest->reg, act->write.mult, (int)value, this->base.verbose  );

    act->write.last_set = (int)value;

    return 0;
}

/**
 * Read value handler  (private)
 */
struct uni_data *modbus_read_data(struct module_object *module, struct modbus_act *act)
{
	if(act->fault_state != FAULT_NONE)
		return uni_data_create_text("error");

    switch(act->atype){
      case ACTTYPE_READ:
		return uni_data_create_float(act->read.last_read);
      case ACTTYPE_FREAD:{
		  unsigned long msec = act->fread.last_read_time; 
		  unsigned long diff = 0;
		  struct timeval time;
		  gettimeofday(&time, NULL);
		  diff = modutil_timeval_diff_ms(&time, &act->fread.last_pulse);
		  if(diff>msec*2)
	      msec= diff;
		  return uni_data_create_flow(act->fread.last_read_pulse,msec);
      }
      case ACTTYPE_WRITE:
		return uni_data_create_int(act->write.last_set);
    } 
	
	return uni_data_create_text("error null");
}

/**
 * Read event handler (public)
 */
struct module_event *modbus_eread(struct event_type *event)
{
     struct modbus_act *act = (struct modbus_act *)event->objdata;
     struct module_object* this = module_get_struct(event->base);
     
     return module_event_create(event, modbus_read_data(this, act), NULL);
}

struct event_handler handlers[] = {{.name = ""}};

/**************************************************/
/* XML parser callbacks                           */
/**************************************************/

int start_act_read(XML_START_PAR)
{
     struct module_object* this = module_get_struct(ele->parent->data);
     struct modbus_act * new = modbus_act_create(this, ACTTYPE_READ, attr);
     assert(new);

     this->acts = modbus_act_add(this->acts, new); // Add to module
     this->base.event_types = event_type_add(this->base.event_types, new->event_type); // make it avaliable
     //  this->reads = modbus_read_add(this->reads, new);

     PRINT_MVB(&this->base, " ");
     return 0;
     
}

int start_act_fread(XML_START_PAR)
{
     struct module_object* this = module_get_struct(ele->parent->data);
     struct modbus_act * new = modbus_act_create(this, ACTTYPE_FREAD, attr);
     assert(new);

     this->acts = modbus_act_add(this->acts, new); // Add to module
     this->base.event_types = event_type_add(this->base.event_types, new->event_type); // make it avaliable
     //  this->reads = modbus_read_add(this->reads, new);

     PRINT_MVB(&this->base, " ");
     return 0;
     
}

int start_act_write(XML_START_PAR)
{
    struct modules *modules = (struct modules*)data;
     struct module_object* this = module_get_struct(ele->parent->data);
     struct modbus_act * new = modbus_act_create(this, ACTTYPE_WRITE, attr);
     struct event_handler *event_handler = NULL;
     assert(new);

     this->acts = modbus_act_add(this->acts, new); // Add to module
     this->base.event_types = event_type_add(this->base.event_types, new->event_type); // make it avaliable
//     this->reads = modbus_read_add(this->reads, new);

     PRINT_MVB(&this->base, " ");

     event_handler = event_handler_create(new->event_type->name,  handler_rcv ,&this->base, (void*)new);
     this->base.event_handlers = event_handler_list_add(this->base.event_handlers, event_handler);

     return module_base_subscribe_event(&this->base, modules->list, get_attr_str_ptr(attr, "event"), FLAG_ALL, event_handler, attr);
     return 0;
     
}

struct xml_tag module_tags[] = {                         \
    { "read" , "module" , start_act_read, NULL, NULL},   \
    { "fread" , "module" , start_act_fread, NULL, NULL},		 \
    { "write" , "module" , start_act_write, NULL, NULL},	 \
    { "" , "" , NULL, NULL, NULL }                       \
};

int module_init(struct module_base *base, const char **attr)
{
    struct module_object* this = module_get_struct(base);
    int status;
    this->fault_event = event_type_create(&this->base, NULL,"fault", 
					  "%", 
					  "Modbus Fault Event", DEFAULT_FFEVT);

    base->event_types = event_type_add(base->event_types, this->fault_event);
    

    status = get_attr_int(attr, "status", 0);
    if(status){
	this->status_event = event_type_create(&this->base, NULL,"status",  "", "Modbus Error Rate", FLAG_AVAI|FLAG_LOG);	
	base->event_types = event_type_add(base->event_types, this->status_event); // make it avaliable    }
    }

    this->fault_state = FAULT_INIT;

    this->tick = module_tick_create(base->tick_master,  base, get_attr_float(attr, "tick", 1), TICK_FLAG_SECALGN1|TICK_FLAG_SKIP);

	this->trxdelay = get_attr_float(attr, "trxdelay", 0)*1000;

	if(this->trxdelay)
		PRINT_MVB(base,"trxdelay %d ms\n", this->trxdelay);
	
    return 0;

}


int module_start(struct module_base *module)
{
    struct module_object* this = module_get_struct(module);

    this->mq_id = connect_ipc();

    flush_msg_ipc(this->mq_id);

    return 0;
}


void module_deinit(struct module_base *module)
{
    struct module_object* this = module_get_struct(module);

    modbus_act_delete(this->acts);

    return;
}

void module_send_event(struct module_object *module, struct event_type *event_type, struct timeval *time, unsigned long diff, float value)
{
    struct uni_data data;
    
    memset(&data, 0, sizeof(data));

    data.type = DATA_FLOAT;
    data.value = (int)value;
    data.mtime = diff;
    data.data = &value;

    module_base_send_event(module_event_create(event_type, &data, time));
}

void module_event_send(struct module_object *module, struct event_type *event_type, struct timeval *time, struct uni_data *data)
{
    module_base_send_event(module_event_create(event_type, data, time));
}



int modbus_act_signaling_test(struct modbus_act *act, float read,  struct timeval *time)
{

    float prev = act->read.last_read ;
    enum act_signaling signaling = act->read.signaling;

    if(act->read.interval_sig)
	if((time->tv_sec) >= ( act->last_update.tv_sec + act->read.interval_sig ))
	    return 1;

    switch(signaling){
      case ACTSIG_ALWAYS:
	return 1;
      case ACTSIG_CHANGE:
	if(read != prev)
	    return 1;
	return 0;
      case ACTSIG_TOHIGH:
	if(read > 0 && prev <= 0)
	    return 1;
	return 0;
      case ACTSIG_TOLOW:
	if(read <= 0 && prev > 0)
	    return 1;
	return 0;
      default:
	return 1;
    } 

 

}



int modbus_reg_read(struct module_object *module, struct modbus_reg *reg, int *value, int *msg_cnt, int *msg_err, int verbose)
{
    int retval = 0;
    int i ;

    for(i=0; i < 3; i++){   
    	*msg_cnt = *msg_cnt+1;
		retval = read_cmd(module->mq_id, reg->mod_id, reg->type, reg->reg, reg->vtype, value, verbose);
		
		if(module->trxdelay)
			usleep(module->trxdelay*1000);

		if(retval==1){
			return FAULT_NONE;
		}
	
		PRINT_ERR("READ ERROR retval %d (%d, %d, %d)\n", retval,reg->mod_id, reg->type, reg->reg );
		*msg_err = *msg_err+1;
    }

    return FAULT_BUS;

}

int modbus_act_read_level(struct module_object *module, struct modbus_act *act, struct timeval *time)
{
      

    int value  = 0;
    float fvalue = 0;

    act->fault_state = modbus_reg_read(module, &act->read.reg, &value, &act->msg_cnt, &act->msg_err, module->base.verbose);

    if(act->fault_state != FAULT_NONE)
		return -1;

    fvalue = module_calc_calc(act->calc, (float)value);
    
    if(act->read.invert){
	if(fvalue > 0)
	    fvalue = 0;
	else
	    fvalue = 1;
    }

    if( modbus_act_signaling_test(act, fvalue, time)){
		module_send_event(module, act->event_type, time, 1, fvalue);
		memcpy(&act->last_update, time, sizeof(struct timeval));
    }

    act->read.last_read = fvalue ;
    memcpy(&act->last_read, time, sizeof(struct timeval));
    

    return 0;       
}

int modbus_act_read_flow(struct module_object *module, struct modbus_act *act, struct timeval *time)
{
    
    float units = 0;
    int ipcount = 0;
    int ptime = 0;
    unsigned long diff = 0;

    act->fault_state = modbus_reg_read(module, &act->fread.reg_pulse, &ipcount, 
				       &act->msg_cnt, &act->msg_err, module->base.verbose);

    if(act->fault_state != FAULT_NONE)
	return -1;
    
    if(ipcount){
		act->fault_state = modbus_reg_read(module, &act->fread.reg_time, &ptime, 
										   &act->msg_cnt, &act->msg_err, module->base.verbose);
		
		if(act->fault_state != FAULT_NONE)
			return -1;
		
		ptime *=10;
    }


    memcpy(&act->last_update, time, sizeof(struct timeval));
    if(ipcount){
	PRINT_MVB(&module->base,"pulse %d \n", ipcount);
	units = act->fread.upp *  ipcount;
	act->fread.last_read_pulse = units;
	act->fread.last_read_time = ptime;
	act->fread.acc_read_pulse += act->fread.last_read_pulse;
	act->fread.acc_read_time += act->fread.last_read_time;

	if(act->fread.last_signal.tv_sec==0){
	    memcpy(&act->fread.last_signal, time, sizeof(struct timeval));
	    act->fread.acc_read_pulse = 0;
	    act->fread.acc_read_time = 0;
	} 

	memcpy(&act->fread.last_pulse, time, sizeof(struct timeval));
    }

    diff = modutil_timeval_diff_ms(time /*now*/, &act->fread.last_signal/*prev*/);   
    
    PRINT_MVB(&module->base,"diff %ld \n", diff);
    if(diff/1000 > act->fread.sig_interval){
	struct uni_data *data ;
	struct timeval  *tv;
	unsigned long sdiff;
	
	if(act->fread.acc_read_pulse){
	    tv = &act->fread.last_pulse;
	    PRINT_MVB(&module->base,"pulse\n");
	} else {
	    tv = time;
	    PRINT_MVB(&module->base,"no\n");
	}

	sdiff = modutil_timeval_diff_ms(tv /*now*/, &act->fread.last_signal/*prev*/);
	data = uni_data_create_flow(act->fread.acc_read_pulse, sdiff );
	
	module_base_send_event(module_event_create(act->event_type, data, tv));
	PRINT_MVB(&module->base,"flow %f  %f %f\n", act->fread.acc_read_pulse, sdiff/(float)1000 ,
		  act->fread.acc_read_pulse/( sdiff/(float)1000)); 
	memcpy(&act->fread.last_signal, tv, sizeof(struct timeval));

	act->fread.acc_read_pulse = 0;
	act->fread.acc_read_time = 0;
    }

    return 0;

}

int modbus_act_read(struct module_object *module, struct modbus_act *act, struct timeval *time)
{
    int retval =  -EFAULT;

    switch(act->atype){
      case ACTTYPE_READ:
	retval = modbus_act_read_level(module, act,time);
	break;
      case ACTTYPE_FREAD:
	retval = modbus_act_read_flow(module, act, time);
	break;
      default:
	return 0;
    } 
    return retval;
}


int modbus_update_fault(struct module_object *this, struct timeval *time)
{
    int new_state = FAULT_NONE;
//    struct module_base *base = &this->base;
    struct modbus_act   *ptr = this->acts;

    while(ptr){
	if(ptr->fault_state > new_state)
	    new_state = ptr->fault_state;
	ptr = ptr->next;
    }


    if(new_state != this->fault_state){
	struct uni_data data;
	PRINT_MVB(&this->base, "fault state changed %d --> %d", this->fault_state ,new_state);
	this->fault_state = new_state;
	memset(&data, 0, sizeof(data));
	data.type = DATA_INT;
	data.value = new_state;
	data.mtime = 1;
	module_base_send_event(module_event_create(this->fault_event, &data, time));
    }

    if((this->status_event)&&(time->tv_sec%3600)==0){
	int total_msg = 0;
	int total_err = 0;
	float errPmsg = 0;
	struct uni_data data;
	
	ptr = this->acts;
	while(ptr){
	    total_msg += ptr->msg_cnt;
	    total_err += ptr->msg_err;
	    ptr->msg_cnt = 0;
	    ptr->msg_err = 0;
	    ptr = ptr->next;
	}
	if(total_msg){
	    errPmsg = (total_err/(float)total_msg)*100;
	    PRINT_MVB(&this->base, "errrate calc %f %ld", errPmsg, time->tv_sec%300);
	    memset(&data, 0, sizeof(data));
	    data.type = DATA_FLOAT;
	    data.value = (float)errPmsg;
	    data.mtime = 1;
	    data.data = &errPmsg;
	    module_base_send_event(module_event_create(this->status_event, &data, time));
	}
    }

    return new_state;

}


void* module_loop(void *parm)
{
    struct module_object *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;

    int retval;
    struct timeval tv;;

    base->run = 1;
    
    while(base->run){ 
        
        struct modbus_act *ptr = this->acts;
        gettimeofday(&tv, NULL);
	
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
				modbus_act_read(this, ptr, &tv);
            }
            ptr = ptr->next;
        }
	modbus_update_fault(this, &tv);

    }
    
    PRINT_MVB(base, "loop returned");
    
    return NULL;

}



struct module_type module_type = {                  \
    .name       = "modbus_read",                       \
    .xml_tags   = module_tags,                      \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct module_object), \
};

struct module_type *module_get_type()
{
    return &module_type;
}
