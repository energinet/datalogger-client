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
 
#ifndef MODULE_TICK_MASTER_H_
#define MODULE_TICK_MASTER_H_

#include "module_tick.h"


/**
 * Init module tic master 
 * @param master The master object
 * @param ms_interval Interval in milliseconds 
 * @param sec_align If 1 align the interval to whole seconds 
 *                  Ex.: Interval 400 unaligned will be  0.000, 0.400, 0.800, 1.000 ... Unaligned it will be  0.000, 0.400, 0.800, 1.200
 */

int module_tick_master_init(struct module_tick_master *master, int ms_interval, int sec_align);

void module_tick_master_stop(struct module_tick_master *master);

int module_tick_master_loop(struct module_tick_master *master, int *run); 

void module_tick_master_set_interval(struct module_tick_master *master, int ms_interval);


#endif /* MODULE_TICK_MASTER_H_ */
