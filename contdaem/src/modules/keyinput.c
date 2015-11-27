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



/**
 * @addtogroup module_xml
 * @{
 * @section keymodxml Key input module
 * Module for handeling key inputs eg /dev/input/event0.
 * <b>Typename: "keyinput" </b>\n
 * <b>Attributes:</b>\n
 * <ul>
 * <li> \b key_start: The first key code\n
 * </ul>
 * <b> Tags: </b>
 * <ul>
 * <li><b>input:</b> Configure input device\n  
 *   - \b device: a path to the input device.
 *   @verbatim  <input device="/dev/input/event0"/> @endverbatim
 * <li><b>count:</b>  Count pulses\n
 *   A event will be transmitted when interval or maxcount is reached.  
 *   If no key events reverived 0 will be transmitted at timeout.\n
 *   - \b key_code: Key code for connected input. Default: is key_start+n where n i number of keys.
 *   - \b upp: Number of units per pulse default: 1.
 *   - \b trig: Thigger on \p high or \p low pulse, or \p all. Default: \p high. 
 @verbatim<count name="pwmtr" text="Power Meter" unit="_J" upp="3600" key_code="0x105" />@endverbatim
 * <li><b>trigger:</b>  start_trigger\n
 *   - \b key_code: Key code for connected input. default: is key_start+n where n i number of keys.
 *   - \b trig: Thigger on \p high or \p low pulse, or \p all. default: \p all.
 *   - \b active: Set weather the input is active \p high og \p low. 
 *        This will invert the input if \p low. Default: \p high
 *   - \b chatter_wait: Milleseconds to wait for chatter cancelation. Default 0 ms.
 @verbatim<trigger text="Button 1" unit="" name="button1" key_code="0x106" flags="nolog,hide" chatter_wait="300"/>@endverbatim
 * <li><b>setup:</b> start_setup \n
 *   - \b timeout: Timeout in seconds before event if no pulse. 
 *   - \b maxcount: Max number of units before event. 
 *   - \b interval: Wait interfal before transmitting events if pulses. 
 * </ul>
 * The key numbers for the three DIN mounted computers are shon in the table below.


 * <TABLE>
 * <TR><TH>Number</TH><TH>#define</TH><TH>SG SDVP</TH><TH>SG Generic</TH><TH>LIAB DIN</TH></TR>
 * <TR><TD> 0x101</TD><TD> BTN_1 </TD><TD>     - </TD><TD>     -    </TD><TD>Button 1</TD></TR>
 * <TR><TD> 0x102</TD><TD> BTN_2 </TD><TD>     - </TD><TD>     -    </TD><TD>Button 2</TD></TR>
 * <TR><TD> 0x103</TD><TD> BTN_3 </TD><TD>     - </TD><TD>     -    </TD><TD> Opto 1 </TD></TR>
 * <TR><TD> 0x104</TD><TD> BTN_4 </TD><TD> Opto 0</TD><TD>  Opto 0  </TD><TD> Opto 2 </TD></TR>
 * <TR><TD> 0x105</TD><TD> BTN_5 </TD><TD> Opto 1</TD><TD>  Opto 1  </TD><TD> Opto 3 </TD></TR>
 * <TR><TD> 0x106</TD><TD> BTN_6 </TD><TD> Button</TD><TD>  Opto 2  </TD><TD> Opto 4 </TD></TR>
 * <TR><TD> 0x107</TD><TD> BTN_7 </TD><TD>    -  </TD><TD>  Opto 3  </TD><TD>    -   </TD></TR>
 * <TR><TD> 0x108</TD><TD> BTN_8 </TD><TD>    -  </TD><TD>  Button  </TD><TD>    -   </TD></TR>
 * </TABLE> 
 * @} 
 */

/**
 * @defgroup modules Available modules
 * @{
 */



#define SEC_UNIT 1000000

enum trigtype {
    TRIG_ALL,
    TRIG_HIGH,
    TRIG_LOW,
};

enum activtype{
    ACTIV_HIGH,
    ACTIV_LOW,
    ACTIV_HIGHOSC,
    ACTIV_LOWOSC,
};


enum keytype {
    KEY_COUNT,
    KEY_TRIGGER,  
};

struct key_count {
    int count;
    int maxcount;
    float upp;  /**<units per pulse */
    unsigned long period;
};


struct key_event {
    int value_sent;
    int ms_wait_chatter;
    enum activtype active;
};

struct key_obj {
    enum keytype type;
    int key_code;
    enum trigtype trig;
    int timeout;
    int interval;
    int value;
    union { 
	struct key_event trigger;
	struct key_count count;
    };
    /**
     * Last received pulse */
    struct timeval last_pulse;
    /**
     * Last transmittet value */
    struct timeval last_reset;
    struct event_type *event_type;
    struct ftolunit *ftlunit;
    struct key_obj *next;
};


/**
 *  @extends module_base
 */

struct keymod_object{
    struct module_base base;
    int fd;
    char *input;
    int timeout;
    int interval;
    int maxpulse;
    int key_code_next;
    struct key_obj *keyobjs;
    int unit_cnt;
};

/**
 * Trigger type sytings 
 * @ref trigtype
 */
const char *trigtyps[] = { "all", "high", "low" , NULL };

/**
 * Input active type strings 
 * @ref activtype
 */
const char *activtyps[] = { "high", "low", "highosc", "lowosc", NULL };

/***********************/
/* Prototypes          */
/***********************/


int key_obj_count_event(struct keymod_object *module, struct key_obj *keyobj,struct timeval *time, int pulse);
int key_obj_trigger_event(struct keymod_object *module, struct key_obj *keyobj,
						  struct timeval *time, int hit_);

/***********************/
/* Utils               */
/***********************/

struct keymod_object* module_get_struct(struct module_base *module){
    return (struct keymod_object*) module;
}


int get_item(const char *text, int def, const char** items, int itemcnt )
{

    return modutil_get_listitem(text, def, items);

    /* int i; */
    /* if(!text) */
    /* 	return def; */
    
    /* for(i = 0; i < itemcnt; i++) { */
    /* 	if(strcmp(text, items[i])==0) */
    /* 	    return i; */
    /* } */

    /* return def; */
}

unsigned long timeval_diff(struct timeval *now, struct timeval *prev)
{
    int sec = now->tv_sec - prev->tv_sec;
    int usec = now->tv_usec - prev->tv_usec;

    unsigned long diff;

    if( sec < 0 ) {
        PRINT_ERR("does not support now < prev");
        sec = 0;
    }

    diff = sec*1000000;
    diff += usec;

    return diff;
    
        
}

/***********************/
/* Key object          */
/***********************/


struct key_obj *key_obj_create(enum keytype type, int key_code, enum trigtype trig, int timeout, int interval)
{
    struct key_obj *new = malloc(sizeof(*new));
    assert(new);
    memset(new, 0, sizeof(*new));
    
    new->type     = type;
    new->key_code = key_code;
    new->trig     = trig;
    new->timeout  = timeout;
    new->interval = interval;

    return new;
}

struct key_obj* key_obj_add(struct key_obj *list,struct key_obj *new){
    struct key_obj *ptr = list;
    
    if(!list)
        return new;
    
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}


void key_obj_delete(struct key_obj *key_obj)
{
    if(!key_obj)
	return ;

    key_obj_delete(key_obj->next);
    free(key_obj);
}


/**
 * get the current level data */
struct uni_data* key_obj_get_ldata(struct key_obj *key_obj)
{
    if(key_obj->type==KEY_TRIGGER){
	//printf("key 0x%x value %d\n",key_obj->key_code,key_obj->value );
	return uni_data_create_int(key_obj->value);
    }else{
	unsigned long usec = key_obj->count.period; 
	unsigned long diff = 0;
	float units = key_obj->count.upp;
	if(usec==0)
	    units = 0;
	else {
	    struct timeval time;
	    gettimeofday(&time, NULL);
	    diff = timeval_diff(&time, &key_obj->last_pulse);

	    if(diff>usec*2) // Update value if no pulse
		usec= diff;
	}
//	printf("key 0x%x units %f time %d\n", key_obj->key_code, units, usec/1000);
	return uni_data_create_flow(units, usec/1000);
    }
}

int key_obj_update(struct keymod_object *module, struct key_obj *keyobj
		   ,struct timeval *time)
{
    switch(keyobj->type){
      case KEY_COUNT:
		return key_obj_count_event(module, keyobj,time, 0);
      case KEY_TRIGGER:
		return key_obj_trigger_event(module, keyobj, time, 0);
      default:
		break;
    }
	
	return 0;
}



int key_obj_value_update(struct keymod_object *module, struct key_obj *keyobj
			 ,struct timeval *time, int value)
{
    int hit = 1;

    switch(keyobj->trigger.active){
      case ACTIV_LOW:
      case ACTIV_LOWOSC:
	keyobj->value = !value;
	break;
      case ACTIV_HIGH:
      case ACTIV_HIGHOSC:
      default:
	keyobj->value = value;
	break;
    }
 
    switch(keyobj->trig){
      case TRIG_HIGH:
	if(value == 0)
	    hit = 0;
	break;
      case TRIG_LOW:
	if(value == 1)
	    hit = 0;
	break;
      case TRIG_ALL:
      default:
	break;
    }
    PRINT_MVB(&module->base, "value %d, hit %d, trig %d", value, hit, keyobj->trig); 
    
    switch(keyobj->type){
      case KEY_COUNT:
	return key_obj_count_event(module, keyobj,time, hit);
      case KEY_TRIGGER:
	return key_obj_trigger_event(module, keyobj, time, hit);
      default:
	break;
    }
    return 0;
}


int key_obj_init_state(struct keymod_object *module, struct key_obj *key_obj)
{
    int key = key_obj->key_code;
    int fd = module->fd;
    int value;
    unsigned char key_b[KEY_MAX/8 + 1];
    
    memset(key_b, 0, sizeof(key_b));
    ioctl(fd, EVIOCGKEY(sizeof(key_b)), key_b);
    
    value = !!(key_b[key/8] & (1<<(key % 8)));

    key_obj_value_update(module, key_obj, NULL, value);//Todo

    return 0;

}





void module_send_event(struct keymod_object *module, struct event_type *event_type, struct timeval *time, unsigned long diff, float value)
{
    struct uni_data *data = uni_data_create_flow(value, diff);
    
    PRINT_MVB(&module->base, "sending %s val %f, diff %lu", event_type->name, value, diff);
   
    module_base_send_event(module_event_create(event_type, data, time));

}

void key_obj_count_reset(struct key_obj *keyobj, struct timeval *time)
{
    memcpy(&keyobj->last_reset, time, sizeof(*time));
    memcpy(&keyobj->last_pulse, time, sizeof(*time));
    keyobj->count.count = 0;
    
}


int key_obj_count_event(struct keymod_object *module, struct key_obj *keyobj,struct timeval *time, int pulse)
{
    unsigned long diff_reset;    
//    unsigned long diff_pulse;    
    
    if(!time)
		return 0;

    if(keyobj->last_reset.tv_sec == 0){
        key_obj_count_reset(keyobj, time);
        return 0;
    }

    diff_reset = timeval_diff(time, &keyobj->last_reset);
//    diff_pulse = timeval_diff(&keyobj->last_pulse, &keyobj->last_reset);
    
    if(pulse){
        if(module->base.verbose)
            fprintf(stderr,".");
        keyobj->count.count++;
	keyobj->count.period = timeval_diff(time, &keyobj->last_pulse);
        memcpy(&keyobj->last_pulse, time, sizeof(*time));
    }

    if(keyobj->count.count && (/*diff_pulse*/ diff_reset > module->interval)){
        unsigned long diff = timeval_diff(&keyobj->last_pulse,&keyobj->last_reset);
        module_send_event(module, keyobj->event_type, &keyobj->last_pulse, diff/1000, 
			    keyobj->count.count*keyobj->count.upp);
        key_obj_count_reset(keyobj, &keyobj->last_pulse);
    } else if(diff_reset > module->timeout){
        unsigned long diff = timeval_diff(time, &keyobj->last_reset);
        module_send_event(module, keyobj->event_type, time, diff/1000, 
			    keyobj->count.count*keyobj->count.upp);
        key_obj_count_reset(keyobj, time);
	keyobj->count.period = 0;
    }
    
    return 0;
    

}

int key_obj_trigger_event(struct keymod_object *module, struct key_obj *keyobj,
			struct timeval *time, int hit_)
{
    struct uni_data *data; 
    int semihit = 0;
    int timediff = 0;
    int hit = hit_;
    int value = keyobj->value;
    
    /* Chatter protection */
    if(keyobj->last_reset.tv_sec&&time){
	timediff = timeval_diff(time, &keyobj->last_reset)/1000;

	if(timediff < keyobj->trigger.ms_wait_chatter)
	    goto out;
    }

    /* interval */
    if(keyobj->interval && time){
	if(((time->tv_sec%keyobj->interval)==0) && time->tv_sec != keyobj->last_reset.tv_sec){
	    semihit = 1;
	    PRINT_MVB(&module->base, "trigger interval %d time %ld", keyobj->interval, time->tv_sec);
	}
    }

    /* timeout */
    if(keyobj->timeout){
	if(timediff > keyobj->timeout*1000){
	    semihit = 1;
	    PRINT_MVB(&module->base, "trigger timeout %d diff %d", keyobj->timeout, timediff);
	}
    }
    
    /* cancle osc active */
    if(( keyobj->trigger.active ==  ACTIV_HIGHOSC || keyobj->trigger.active == ACTIV_LOWOSC)&&time){
	int pulsediff = timeval_diff(time, &keyobj->last_pulse)/1000;
	if(pulsediff < keyobj->trigger.ms_wait_chatter){
	    if(keyobj->trigger.value_sent == 1 ){
		if(hit)
		    PRINT_MVB(&module->base, "osc on diff %d < %d , (%d --> %d)", pulsediff, 
			      keyobj->trigger.value_sent, keyobj->value, keyobj->trigger.ms_wait_chatter);
		hit = 0;
		value = 1;

	    }
	}
    }


    if(semihit || hit || keyobj->trigger.value_sent != value ){
	PRINT_MVB(&module->base, "value %d --> sent %d\n", keyobj->trigger.value_sent , value);
	if(time){
	    memcpy(&keyobj->last_reset, time, sizeof(*time));
	} else {
	    gettimeofday(&keyobj->last_reset, NULL);
	}
		
	data = uni_data_create_int(value);
	data->mtime = timediff;
	
	module_base_send_event(module_event_create(keyobj->event_type, 
						   data, &keyobj->last_reset));
	
	keyobj->trigger.value_sent = value ;

    }

  out:

    if(hit_){
	if(time){
	    memcpy(&keyobj->last_pulse, time, sizeof(*time));
	} else {
	    gettimeofday(&keyobj->last_pulse, NULL);
	}
    }
    
    return 0;

}

struct module_event *key_obj_eread(struct event_type *event)
{
     struct key_obj *keyobj = (struct key_obj *)event->objdata;
     //  printf("-->%p\n", keyobj);
     
     return module_event_create(event, key_obj_get_ldata(keyobj), &keyobj->last_reset);
 }
struct event_handler handlers[] = {{.name = ""}};



int start_input(XML_START_PAR)
{
    struct keymod_object* this = module_get_struct(ele->parent->data);
    const char *device = get_attr_str_ptr(attr, "device"); 

    if(device)
        this->input = strdup(device);

    return 0;
}


int start_setup(XML_START_PAR)
{
    struct keymod_object* this = module_get_struct(ele->parent->data);
    

    this->timeout  = SEC_UNIT*get_attr_int(attr, "timeout" , this->timeout/SEC_UNIT  );
    this->interval  = SEC_UNIT*get_attr_int(attr, "interval" , this->interval/SEC_UNIT  );
    this->maxpulse = get_attr_int(attr, "maxpulse", this->maxpulse );

    return 0;
}

int start_input_obj(XML_START_PAR)
{
    struct keymod_object* this = module_get_struct(ele->parent->data);
    

    this->timeout  = SEC_UNIT*get_attr_int(attr, "timeout" , this->timeout/SEC_UNIT  );
    this->interval  = SEC_UNIT*get_attr_int(attr, "interval" , this->interval/SEC_UNIT  );
    this->maxpulse = get_attr_int(attr, "maxpulse", this->maxpulse );

    return 0;
}





int start_add_event_type(struct keymod_object* module, struct key_obj *key_obj, const char **attr)
{
        
    key_obj->event_type =  event_type_create_attr(&module->base, (void*)key_obj,attr);
    
    if(!key_obj->event_type){
        PRINT_ERR("event_type is NULL");
        return -1;
    }    

    key_obj->event_type->read =  key_obj_eread;

    /* if(key_obj->type == KEY_COUNT){ */
    /* 	key_obj->ftlunit = module_calc_get_flunit(get_attr_str_ptr(attr, "unit"), NULL); */
    /* 	if(key_obj->ftlunit){ */
    /* 	    key_obj->read_event_type = event_type_create(&module->base,key_obj,  */
    /* 							 key_obj->event_type->name,  key_obj->ftlunit->lunit,   */
    /* 							 get_attr_str_ptr(attr, "text"), 0); */
    /* 	}else { */
    /* 	    key_obj->read_event_type = event_type_create(&module->base,key_obj,  */
    /* 							 key_obj->event_type->name, "jpp",  */
    /* 							 get_attr_str_ptr(attr, "text"), 0); */
    /* 	}	 */
    /* } */
    
    module->base.event_types = event_type_add(module->base.event_types, key_obj->event_type);

    PRINT_MVB(&module->base, "addinged event:%p %s.%s",key_obj->event_type, module->base.name, key_obj->event_type->name);

    PRINT_MVB(&module->base, "first event:%p %s.%s",module->base.event_types,
	      module->base.name, module->base.event_types->name);

      return 0;
    
}


int start_count(XML_START_PAR)
{
    struct keymod_object* module = module_get_struct(ele->parent->data);
    struct key_obj *new = NULL;
    enum trigtype trig = (enum trigtype)get_item(
	get_attr_str_ptr(attr, "trig"), 
	TRIG_HIGH,trigtyps, 3);

    printf("trig is %s (%d) --> %s\n", get_attr_str_ptr(attr, "trig"), trig, trigtyps[trig]);

    new = key_obj_create(KEY_COUNT, 
			 get_attr_int(attr, "key_code", module->key_code_next++)
			 , /*TRIG_HIGH*/ trig,  get_attr_int(attr, "timeout", module->timeout), 0);

    start_add_event_type(module, new, attr);


    new->count.maxcount = get_attr_int(attr, "maxcount", module->maxpulse);
    new->count.upp      = get_attr_float(attr, "upp", 1 );

    module->keyobjs = key_obj_add(module->keyobjs , new);

    PRINT_MVB(&module->base, "adding input count: %s, upp %f, key: 0x%x, trig: %s", 
	      new->event_type->name, new->count.upp, new->key_code, trigtyps[new->trig]);

    return 0;
}




int start_trigger(XML_START_PAR)
{
    struct keymod_object* module = module_get_struct(ele->parent->data);
    struct key_obj *new = NULL;
    enum trigtype trig = (enum trigtype)get_item(
	get_attr_str_ptr(attr, "trig"), 
	TRIG_ALL,trigtyps, 3);

    enum activtype active = (enum activtype)get_item(
	get_attr_str_ptr(attr, "active"), 
	ACTIV_HIGH,activtyps, 4);


    new = key_obj_create(KEY_TRIGGER, 
			 get_attr_int(attr, "key_code", module->key_code_next++)
			 , trig,  get_attr_int(attr, "timeout", 0), get_attr_int(attr, "interval", 0));

    new->trigger.active = active;

    new->trigger.ms_wait_chatter = get_attr_int(attr, "chatter_wait", 0);

    start_add_event_type(module, new, attr);

    module->keyobjs = key_obj_add(module->keyobjs , new);

    PRINT_MVB(&module->base, 
	      "adding input trigger: %s, key: 0x%x, trig: %s active %s", 
	      new->event_type->name, 
	      new->key_code, trigtyps[new->trig], 
	      activtyps[new->trigger.active]);

    return 0;
}

struct xml_tag module_tags[] = {                         \
    { "input"  , "module" , start_input, NULL, NULL},   \
    { "count"  , "module" , start_count, NULL, NULL},   \
    { "trigger", "module" , start_trigger, NULL, NULL},   \
    { "setup"  , "module" , start_setup, NULL, NULL}, \
    { "" , ""  , NULL, NULL, NULL }                       \
};
    

int module_init(struct module_base *module, const char **attr)
{
    struct keymod_object* this = module_get_struct(module);
    
    this->timeout = 600*SEC_UNIT; /* 10 min */
    this->interval = 300*SEC_UNIT; /* 5 min */
    this->maxpulse = 100; 
    this->fd = -1;
    this->key_code_next =  get_attr_int(attr, "key_start", 0x108) ;

    return 0;
}

int module_start(struct module_base *module)
{
    struct keymod_object* this = module_get_struct(module);

    if(!this->input){
        PRINT_ERR("No input path");
        return -1;
    }
    
    this->fd = open(this->input, O_RDWR);

    if(this->fd <0){
        PRINT_ERR("Error opening input: %s", strerror(errno));
        return -1;
    }
      
  
    PRINT_MVB(&this->base, "first event:%p %s.%s",this->base.event_types,
	      this->base.name, this->base.event_types->name);

    return 0;
}


void module_deinit(struct module_base *module)
{
    struct keymod_object* this = module_get_struct(module);

    if(this->fd >= 0)
        close(this->fd);

    return;
}

#define POLL_TIMEOUT 100

void input_event_print(struct input_event *event){
    printf("input event: time %ld.%6.6ld, type: %x, code %x, value %x\n", 
           event->time.tv_sec, event->time.tv_usec, event->type, event->code, 
           event->value);
}


int key_obj_pulse(struct keymod_object *module, struct key_obj *keyobj
			 ,struct timeval *time, struct input_event *event)
{

    if(event ){
		if(keyobj->key_code == event->code){
			key_obj_value_update(module, keyobj, time,  event->value);
//	    PRINT_MVB(&module->base, "value %d, hit %d, trig %d", value, hit, keyobj->trig); 
		}

    } else {
		key_obj_update(module, keyobj,time);
    }

    return 0;
}


void* module_loop(void *parm)
{
    struct keymod_object *this = module_get_struct(parm);
    struct module_base *base = ( struct module_base *)parm;
    struct key_obj *ptr = this->keyobjs;
    struct pollfd poll_st;
    struct input_event event;
    int retval;
    base->run = 1;
    PRINT_MVB(base, "start loop\n");

    while(ptr){
	key_obj_init_state(this, ptr);
	ptr = ptr->next;
    }

    while(base->run){ 
        poll_st.fd        = this->fd;
        poll_st.events    = POLLIN;
        retval = poll(&poll_st, 1, POLL_TIMEOUT);
        if(retval > 0 && poll_st.revents == POLLIN){
            int cnt = 0;
            cnt = read(this->fd, &event, sizeof(struct input_event));
//            printf(" event.code %x,\n",  event.code);
	    //   fprintf(stderr, ".");
	    //PRINT_MVB(base, " event.code %x,\n", event.code);
            if(cnt > 0){
                if(event.type == EV_KEY) {
                    ptr = this->keyobjs;
                    while(ptr){
			key_obj_pulse(this, ptr , &event.time, &event);
                        ptr = ptr->next;
                    }
                }
            } 
        } else {
	    struct timeval time;
	    ptr = this->keyobjs;
	    gettimeofday(&time, NULL);
            while(ptr){
		key_obj_pulse(this, ptr , &time, NULL);
		ptr = ptr->next;
            }
        }

    } 
    
    PRINT_MVB(base, "loop returned");
    
    return NULL;

}

struct module_type module_type = {                  \
    .name       = "keyinput",                       \
    .xml_tags   = module_tags,                      \
    .handlers   = handlers ,                        \
    .type_struct_size = sizeof(struct keymod_object), \
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
