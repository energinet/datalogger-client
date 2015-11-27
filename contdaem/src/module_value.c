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

#include "module_value.h"
#include "module_base.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <sys/time.h>
#include <assert.h>
#include <limits.h>

void evalue_callback_run(struct evalue* evalue);

/**
 * Read event handler
 */
struct module_event *evalue_eread(struct event_type *event)
{
     struct evalue* evalue = (struct evalue* )event->objdata;

     float value = evalue_getf(evalue);

     return module_event_create(event, uni_data_create_float(value), NULL);
 }


/**
 * Write event handler
 */
int evalue_ewrite(struct event_type *event, struct uni_data *data)
{
    struct evalue* evalue = (struct evalue* )event->objdata;

    evalue_setf(evalue, uni_data_get_fvalue(data));
    
    return 0;
}



int is_text(const char *str)
{
    int val_len, i;
    val_len = strlen(str);

    for(i = 0; i < val_len; i++){
        char c = str[i];
        if(c >= '0' && c <= '9')
            continue;
        else if (c == ',' || c == '.'|| c == '-' || c == ':'){
            continue;
        } else {
            return 1;
        }
    }
    
    return 0;
}





void evalue_init(struct evalue* evalue, struct module_base *base, const char *name,  
		 const char *subname, const char *param, unsigned long flags, const char *unit, const char *hname)
{
    char vname[256];  
    
    assert(evalue);

    if(subname)
	snprintf(vname, sizeof(vname), "%s_%s", name, subname);
    else
	snprintf(vname, sizeof(vname), "%s", name);
    
    memset(&evalue->time, 0, sizeof(struct timeval));

    evalue->name = strdup(vname);
    evalue->base = base;
    //evalue->value = 0; /* obs */ /* ToDo: init uni_data */
    evalue->uvalue = NULL; /* ToDo: Test */
    evalue->callback = NULL;

    if(param){
	if(is_text(param)){
	    char event[256];
	    char *value = strchr(param, '#');
	    
	    if(value){
		int len = value-param;
		memcpy(event, param, len);
		event[len] = '\0';
		value++;
		//sscanf(value, "%f", &evalue->value); /* obs */ /* ToDo: Erstatte med uni_data_create_from_str agtig */
		evalue->uvalue = uni_data_create_from_typstr(value); /* ToDo: Test */
	    }else{
		//evalue->value = 0; /* obs */ /* ToDo: Erstatte med uni_data_create_from_str agtig */
		evalue->uvalue = uni_data_create_float(0); /* ToDo: test */
		strcpy(event, param);
	    }
	    
	    PRINT_MVB(base, "'%s' '%s'", event, value);
	    evalue_subscribe(evalue, event, FLAG_ALL);
	} else {
	    
	    //			sscanf(param, "%f", &evalue->value); /* obs */
	    evalue->uvalue = uni_data_create_from_typstr(param); /* ToDo: Test */
	}
    }
    
    if(!evalue->uvalue)
	evalue->uvalue = uni_data_create_float(0); /* ToDo: test */
    
    evalue->event_type = event_type_create(base, evalue, vname, unit, hname,flags);

    assert(evalue->event_type);

    evalue->event_type->write = evalue_ewrite;
    evalue->event_type->read = evalue_eread;

    pthread_mutex_init(&evalue->mutex, NULL);
    evalue->cond_use = 0;
    pthread_cond_init(&evalue->cond, NULL);

}


struct evalue* evalue_create_attr(struct module_base *base, const char *param, 
				  const char **attr){
    struct evalue* new = malloc(sizeof(*new));
    assert(new);
    
    evalue_init(new, base, get_attr_str_ptr(attr, "name"), NULL, param,
		event_type_get_flags(get_attr_str_ptr(attr, "flags"), base->flags), 
		get_attr_str_ptr(attr, "unit"), 
		get_attr_str_ptr(attr, "text"));

    base->event_types = event_type_add(base->event_types, new->event_type);

    PRINT_MVB(base,"Created value %s, set value %f from attr and added\n",  
	      new->name,  evalue_getf(new));    
    
    

    return new;
}


struct evalue* evalue_create_ext(struct module_base *base, const char *name,  const char *subname, 
				 const char *param , unsigned long flags, const char *unit, const char *hname)
{
    
    struct evalue *new = NULL;
    
  
    new = malloc(sizeof(*new));
    assert(new);

    evalue_init(new, base, name,  subname, param, flags, unit, hname);

    PRINT_MVB(base,"Created value %s, set value %f\n",  new->name,  evalue_getf(new));    

    return new;
}

struct evalue* evalue_create(struct module_base *base, const char *name,  const char *subname,
			     const char *param , unsigned long flags){
    if(!param){
	PRINT_MVB(base,"Could not create evalue %s_%s: No param\n",  name, subname);    
	return NULL;
    }

    return evalue_create_ext(base, name,  subname, param, flags, NULL, NULL);
}


void evalue_delete(struct evalue* evalue)
{
	if(!evalue)
		return;

	pthread_mutex_destroy(&evalue->mutex);
	
	free(evalue->name);
	uni_data_delete(evalue->uvalue); /* ToDo: Test */
    
	free(evalue);
}



float evalue_getf(struct evalue* evalue)
{
    float value = 0;

    if(!evalue)
	 return 0;

    pthread_mutex_lock(&evalue->mutex);

    //value = evalue->value; /* obs */
	value = uni_data_get_fvalue(evalue->uvalue); /* ToDo: Test */

    pthread_mutex_unlock(&evalue->mutex);

    return value;
}

const char *evalue_unit(struct evalue* evalue)
{
    if(!evalue)
        goto na;

    if(!evalue->event_type)
        goto na;

    if(evalue->event_type->unit)
        return evalue->event_type->unit;
    
 na:
    return "";
}

struct uni_data *evalue_getdata(struct evalue* evalue)
{
	struct uni_data *data;
    
	if(!evalue)
		return 0;
	
    pthread_mutex_lock(&evalue->mutex);
	
	data =  uni_data_copy(evalue->uvalue);

	pthread_mutex_unlock(&evalue->mutex);

	return data;
}

void evalue_set__(struct evalue* evalue, struct timeval *now)
{
    struct timeval now_;
	
    if(!now){
		gettimeofday(&now_, NULL);
		now = &now_;
    }

	memcpy(&evalue->time, now, sizeof(struct timeval));
	
	if(evalue->cond_use > 0)
		pthread_cond_broadcast(&evalue->cond);
}


/**
 * Set evalue by uni_data 
 * ToDo: Test.
 */

void evalue_setdata(struct evalue* evalue, struct uni_data *data, struct timeval *now)
{
	struct uni_data *outdata;
	
	pthread_mutex_lock(&evalue->mutex);

	uni_data_set_data(evalue->uvalue, data);

	outdata = uni_data_copy(evalue->uvalue); /* ToDo: Test */

	evalue_set__(evalue, now);	

	pthread_mutex_unlock(&evalue->mutex);
	
    module_base_send_event(module_event_create(evalue->event_type, outdata, &evalue->time));  
}

/**
 * Seat float (level) value with time
 */
void evalue_setft(struct evalue* evalue, float value, struct timeval *now)
{
    struct uni_data *outdata;
 
    pthread_mutex_lock(&evalue->mutex);
   
	uni_data_set_fvalue(evalue->uvalue, value);  /* ToDo: Test */

	outdata = uni_data_copy(evalue->uvalue); /* ToDo: Test */

	evalue_set__(evalue, now);	

    pthread_mutex_unlock(&evalue->mutex);

    module_base_send_event(module_event_create(evalue->event_type, outdata, &evalue->time));  

}

/**
 * Seat float (level) value with time now
 */
void evalue_setf(struct evalue* evalue, float value)
{
    evalue_setft(evalue, value, NULL);
}


/* void evalue_setdata(struct evalue* evalue, struct uni_data *data, struct timeval *now) */
/* { */
/* 	evalue_setdatat(struct evalue* evalue, struct uni_data *snow) */
/*     evalue_setft(evalue, uni_data_get_fvalue(data), now); */
/* } */

int evalue_isvalid(struct evalue* evalue)
{
  if((evalue->time.tv_sec != 0) || (evalue->time.tv_usec != 0))
    return 1;
  return 0;
}

double evalue_elapsed(struct evalue* evalue, struct timeval *now)
{
    struct timeval now_;
    struct timeval *upd  = &evalue->time;

    if(now == NULL){
	gettimeofday(&now_, NULL);
	now = &now_;
    }

    double now_ms , upd_ms , diff;
     
    now_ms = (double)now->tv_sec*1000000 + (double)now->tv_usec;
    upd_ms = (double)upd->tv_sec*1000000 + (double)upd->tv_usec;
     
    diff = (double)now_ms - (double)upd_ms;
     
    return diff/1000000;

}

int evalue_handler_rcv(EVENT_HANDLER_PAR)
{
    struct evalue* evalue = (struct evalue*)handler->objdata;
    struct uni_data *data = event->data;
	PRINT_MVB(evalue->base, "Setting value");
    evalue_setdata(evalue, data, &event->time);
	PRINT_MVB(evalue->base, "Running callbacks (callback %p)",evalue->callback );
    evalue_callback_run(evalue);
    return 0;
}


int evalue_subscribe(struct evalue* evalue, const char *event_name, unsigned long  mask)
{
    struct event_handler *event_handler = NULL;
    struct module_base *base = evalue->base;
    struct event_search search;
    struct event_type *event;
    int count = 0;
    int retval = 0;

    event_handler = event_handler_create(evalue->name,  evalue_handler_rcv ,base, (void*)evalue);

    base->event_handlers = event_handler_list_add(base->event_handlers, event_handler);

    event_search_init(&search, base->first,event_name, mask);

    while((event = event_search_next(&search))){
	if((retval = module_base_subscribe(base, event, event_handler))<0)
	    return retval;

	count++;
    }

    if(!count)
	PRINT_ERR("Could not find '%s'", event_name);

    return count;

}


void evalue_callback_set(struct evalue* evalue, void (*callback)(EVAL_CALL_PAR), void* userdata)
{
    pthread_mutex_lock(&evalue->mutex);
    
    evalue->callback = callback;
    evalue->userdata = userdata;

    pthread_mutex_unlock(&evalue->mutex);
}

void evalue_callback_clear(struct evalue* evalue)
{
    pthread_mutex_lock(&evalue->mutex);

    evalue->callback = NULL;

    pthread_mutex_unlock(&evalue->mutex);
}


void evalue_callback_run(struct evalue* evalue)
{
    void (*callback)(EVAL_CALL_PAR);

    pthread_mutex_lock(&evalue->mutex);

    callback = evalue->callback;

    pthread_mutex_unlock(&evalue->mutex);    

    if(callback)
	callback(evalue);

} 



int evalue_insync(struct evalue* evalueA, struct evalue* evalueB, 
		  float *valueA, float *valueB, struct timeval *stime)
{
    int insync = 0;

    pthread_mutex_lock(&evalueA->mutex);
    pthread_mutex_lock(&evalueB->mutex);

    if(valueA)
		*valueA = uni_data_get_fvalue(evalueA->uvalue);// evalueA->value; /* obs */
    
    if(valueB)
		*valueB = uni_data_get_fvalue(evalueB->uvalue);//evalueB->value; /* obs */

    if((evalueA->time.tv_sec==evalueB->time.tv_sec)&&
       (evalueA->time.tv_usec==evalueB->time.tv_usec)){
		insync = 1; 
		if(stime)
			memcpy(stime,&evalueA->time, sizeof(struct timeval));
    }
    
    pthread_mutex_unlock(&evalueA->mutex);
    pthread_mutex_unlock(&evalueB->mutex);
    
    return insync;
    
}


int evalue_wait__(struct evalue* evalue, int timeout)
{
	int ret;

	if(evalue->cond_use < 0)
		return -EFAULT;

	evalue->cond_use++;

	if(timeout){
		struct timespec ts_timeout;
		clock_gettime(CLOCK_REALTIME, &ts_timeout);
		ts_timeout.tv_sec += timeout/1000;
		ts_timeout.tv_nsec += (timeout%1000)*1000000;
		ret = pthread_cond_timedwait(&evalue->cond, &evalue->mutex, &ts_timeout);
	} else {
		ret = pthread_cond_wait(&evalue->cond, &evalue->mutex);
	}

	evalue->cond_use--;

	return ret;
	
}



int evalue_wait(struct evalue* evalue, int timeout)
{
	int ret;

	pthread_mutex_lock(&evalue->mutex);
	ret = evalue_wait__(evalue, timeout);
	pthread_mutex_unlock(&evalue->mutex);

	return ret;
}



int evalue_wait_value(struct evalue* evalue, float value, int timeout)
{
	int ret = 0;

	pthread_mutex_lock(&evalue->mutex);
	while(uni_data_get_fvalue(evalue->uvalue) != value){
		fprintf(stderr, "value %f\n", uni_data_get_fvalue(evalue->uvalue) );
		if((ret = evalue_wait__(evalue, timeout))){
			break;
		}
		   
	}
	
	pthread_mutex_unlock(&evalue->mutex);
	
	return ret;
}


void evalue_stopwait(struct evalue* evalue)
{
	pthread_mutex_lock(&evalue->mutex);
	evalue->cond_use = -1;
	pthread_cond_broadcast(&evalue->cond);
    pthread_mutex_unlock(&evalue->mutex);

}
