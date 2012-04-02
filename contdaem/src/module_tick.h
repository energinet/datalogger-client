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
 
#ifndef MODULE_TICK_H_
#define MODULE_TICK_H_
#include <pthread.h>
#include <sys/time.h>

#define TICK_FLAG_SKIP (1 << 8)
#define TICK_FLAG_SECALGN 0xf
#define TICK_FLAG_SECALGN1 1
#define TICK_FLAG_SECALGN2 2

struct module_tick;

struct module_tick_master{
    pthread_t thread;  
    pthread_mutex_t mutex;
    pthread_cond_t cond;   
    struct timeval time;
    unsigned long ms_interval;
    int sec_align;
    unsigned long seq;
    int rollerr;  /*< Master did not fire befire next tick */
    int lateerr;  /*< Master did not fire within 20 ms */
    int notinerr; /*< Tick was not in */
    int dbglev;
    int outcnt;
    int run;
    struct module_tick *ticks;
};


struct module_tick{
    float interval;
    int div;
    unsigned long seq;
    unsigned long div_seq;
    unsigned long rollerr;
    unsigned long outerr;
    int inwait;
    unsigned long flags;
    struct module_tick_master *master;
    struct module_tick *next;
};




struct module_tick *module_tick_create(struct module_tick_master *master, float interval, unsigned long flags);

struct module_tick *module_tick_add(struct module_tick *list, struct module_tick *new);
struct module_tick *module_tick_rem(struct module_tick *list, struct module_tick *rem);

void module_tick_delete(struct module_tick *tick);

void module_tick_set_interval(struct module_tick *tick, float interval);

/**
 * Wait for at tick
 */
int module_tick_wait(struct module_tick *tick, struct timeval *time);


/**
 * Wait for at tick
 */
int module_tick_wait_(struct timeval *time, unsigned long *seq );

void module_tick_master_set_interval(struct module_tick_master *master, int ms_interval);

#endif /* MODULE_TICK_H_ */
