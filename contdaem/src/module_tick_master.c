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


long module_tick_ts_diff(struct timespec *ts_a, struct timespec *ts_b)
{
    
    long nsec = (ts_a->tv_nsec - ts_b->tv_nsec);

    nsec += (ts_a->tv_sec - ts_b->tv_sec)*1000000000;

    return nsec;
}



void module_tick_master_upd_out(struct module_tick *list)
{
    struct module_tick *ptr = list;

    while(ptr){
	if(!ptr->inwait)
	    ptr->outerr++;
	ptr = ptr->next;
    }
	
}

int module_tick_master_loop(struct module_tick_master *master, int *run)
{

    pthread_mutex_t mutex;
    pthread_cond_t cond;   
    struct timespec ts_next;
    struct timespec ts_now;
    int dbglev = master->dbglev;
    long ns_diff = 0;
    unsigned long ns_interval = master->ms_interval*1000000;
    int sec_align = master->sec_align;
    unsigned long seq = 0;

    if(dbglev)
	fprintf(stderr, "start tick interval %lu ns\n", ns_interval);

    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);

    clock_gettime(CLOCK_REALTIME, &ts_next);
    ts_next.tv_nsec = 0;
    ts_next.tv_sec += 1;

    while(*run){
	


	if(ts_next.tv_nsec >= 1000000000){
	    ts_next.tv_sec += 1;
	    ts_next.tv_nsec -= 1000000000;
	} 
	
	if(sec_align&&(ts_next.tv_nsec< ns_interval))
	    ts_next.tv_nsec = 0;

	pthread_cond_timedwait(&cond, &mutex, &ts_next);

	pthread_mutex_lock(&master->mutex);
	clock_gettime(CLOCK_REALTIME, &ts_now);
	master->seq = seq++;
	master->time.tv_sec = ts_now.tv_sec;
	master->time.tv_usec = ts_now.tv_nsec/1000;
	pthread_cond_broadcast(&master->cond);
	if(master->outcnt){
	    module_tick_master_upd_out(master->ticks);
	    master->notinerr += master->outcnt;
	    fprintf(stderr, "out count %d\n", master->outcnt); 
	}
	pthread_mutex_unlock(&master->mutex);
	
	ns_diff = module_tick_ts_diff(&ts_now, &ts_next);

	if(ns_diff > 20000000){
	    master->lateerr++;
	    if(ns_diff > ns_interval){
		master->rollerr++;
		fprintf(stderr, "rollover\n"); 
	    }
	}

	ts_next.tv_nsec += ns_interval;
	
	if(dbglev)
	    fprintf(stderr, "tick %ld %ld %ld.%3.3ld\n", seq-1, ns_diff/1000000, ts_now.tv_sec, ts_now.tv_nsec/1000000); 
    }

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);

    return 0;
    
}




int module_tick_master_init(struct module_tick_master *master, int ms_interval, int sec_align)
{
    fprintf(stderr, "init tick master %p\n", master);

    assert(master);
    memset(master , 0 , sizeof(*master));
    
    master->run = 1;

    module_tick_master_set_interval(master, ms_interval);
    master->sec_align = sec_align;
    master->ticks = NULL;
    pthread_cond_init(&master->cond, NULL);
    pthread_mutex_init(&master->mutex, NULL);
    fprintf(stderr, "tick master created %p\n", master);
    return 0;

}

void module_tick_master_stop(struct module_tick_master *master)
{
    void* retptr = NULL;
    int retval = 0;
    
    fprintf(stderr, "stop tick master %p\n", master);

    master->run = 0;

    retval = pthread_join(master->thread, &retptr);

    if(retval < 0)
            PRINT_ERR("pthread_join returned %d in tick master : %s", retval, strerror(-retval));
    
    PRINT_DBG(1, "pthread joined"); 
 
    pthread_cond_destroy(&master->cond);
    pthread_mutex_destroy(&master->mutex);
   

}

