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

#include "module_tick.h"
#include "contdaem.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

struct module_tick *module_tick_create(struct module_tick_master *master, struct module_base *base, float interval, unsigned long flags)
{
    struct module_tick *new = malloc(sizeof(*new));
    assert(new);
    fprintf(stderr, "createing tick: %p, master %p, interval %f\n", new, master, interval);

    memset(new , 0 , sizeof(*new));
    new->master = master;
    new->flags = flags;
    module_tick_set_interval(new, interval);

    // fprintf(stderr, "tick created: div %d, interval %f\n", new->div, interval);

    pthread_mutex_lock(&master->mutex);
    //fprintf(stderr, "adding %p to master %p\n", new, master);
    master->ticks = module_tick_add(master->ticks, new);
    //fprintf(stderr, "added %p\n", new);

    new->div_seq = master->seq - new->div;
    
    if(TICK_FLAG_SECALGN&flags){
	new->div_seq += (TICK_FLAG_SECALGN&flags)-1;
    }

    master->outcnt++;
    new->inwait = 0;

	new->base = base;

    pthread_mutex_unlock(&master->mutex);
    
//    fprintf(stderr, "tick created: div %d, interval %f\n", new->div, interval);

    return new;

}

struct module_tick *module_tick_add(struct module_tick *list, struct module_tick *new)
{
    struct module_tick *ptr = list;
    
    if(!ptr){
	return new;
    }
      
    while(ptr->next)
	ptr = ptr->next;
    
    ptr->next = new;
    
    return list;

}

struct module_tick *module_tick_rem(struct module_tick *list, struct module_tick *rem)
{

    struct module_tick *ptr = list;
    
    if(!rem){
	return list;
    }
    
    if(ptr == rem){
	struct module_tick *next = ptr->next;
	ptr->next = NULL;
	return next;
    }

    while(ptr->next){
	if(ptr->next == rem){
	    struct module_tick *found = ptr->next;
	    ptr->next = found->next;
	    found->next = NULL;
	    break;
	}
	ptr = ptr->next;
    }
    

    return list;
    
}


void module_tick_delete(struct module_tick *tick)
{
    struct module_tick_master *master = tick->master;
    
    pthread_mutex_lock(&master->mutex);
    master->outcnt--;
    master->ticks = module_tick_rem(master->ticks, tick);
    pthread_mutex_unlock(&master->mutex);

    free(tick);

}

void module_tick_set_interval(struct module_tick *tick, float interval)
{
    struct module_tick_master *master = tick->master;

    tick->interval = interval;

    if(interval != 0)
	tick->div = (interval*1000)/(master->ms_interval);
    else 
	tick->div = 1;

    fprintf(stderr, "Set tick interval set to %f (div %d)\n", interval, tick->div);
}



int module_tick_wait(struct module_tick *tick, struct timeval *time)
{
    
    struct module_tick_master *master = tick->master;
    int retval = 0;    

    if(!master)
	return -1;

    if(!master->run)
	return -1;

    if(!tick)
	return -1;

    pthread_mutex_lock(&master->mutex);
    master->outcnt--;
    tick->inwait = 1;

    do{
	retval = pthread_cond_wait(&master->cond, &master->mutex);
	//fprintf(stderr, "tick %p %ld %ld %ld %d  -- %ld.%3.3ld \n", tick, master->seq , tick->div_seq , master->seq - tick->div_seq, retval,
	//	master->time.tv_sec, master->time.tv_usec/1000); 
    }while((retval == 0) && ((master->seq - tick->div_seq) < tick->div));
    
    tick->inwait = 0;
    master->outcnt++;
    
    memcpy(time, &master->time, sizeof(struct timeval));
    tick->seq = master->seq;
        
    pthread_mutex_unlock(&master->mutex);

    tick->div_seq += tick->div;

    if(tick->seq > tick->div_seq){
	tick->rollerr++;
	
	if(tick->flags&TICK_FLAG_SKIP){
	    if((tick->div == 1)||!(tick->flags&TICK_FLAG_SECALGN)){
		retval = (tick->seq-tick->div_seq)/tick->div;
		tick->div_seq = tick->seq;
	    } else {
		retval = 0;
		while(tick->seq > tick->div_seq){
		    tick->div_seq += tick->div;
		    retval++;		    
		}
	    }
	}

    }

//    fprintf(stderr, "tick %p %ld %ld %ld.%3.3ld\n", tick, tick->seq,  tick->div_seq, time->tv_sec, time->tv_usec/1000); 
    
    
    return retval;

}




void module_tick_master_set_interval(struct module_tick_master *master, int ms_interval)
{
    struct module_tick *ptr = master->ticks;

    fprintf(stderr, "Set tick master interval to %d ms\n", ms_interval);

    master->ms_interval = ms_interval;
    while(ptr){
	module_tick_set_interval(ptr, ptr->interval);
	ptr = ptr->next;
    }

}
