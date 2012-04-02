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

#ifndef TIMEVAR_H_
#define TIMEVAR_H_
#include <time.h>

struct timevar{
    /**
     * Current value */
    char value[128];
    /**
     * Time of last update */
    time_t t_update;
    /**
     * Update time of last read value*/
    time_t t_read;
};


/**
 * Init time variable with name 
 * @param var variable object
 * @param value first value
 */
void timevar_init(struct timevar *var, const char *value);

/**
 * Set the timevar
 * @param var variable object
 * @param value setvalue 
 * @param t_update variable update time 
 */
void timevar_set(struct timevar *var, const char *value, time_t t_update);


/**
 * Set the timevar
 * @param var variable object
 * @param value setvalue 
 * @param t_update variable update time 
 */
void timevar_setf(struct timevar *var, float value, time_t t_update);

/**
 * Read variable
 * @param var variable object
 * @param buf buffer fir return value 
 */
void timevar_read(struct timevar *var, char *buf, int buf_size);

/**
 * Read float vaiable
 * @param var variable object
 * @return value
 */
float timevar_readf(struct timevar *var);

/**
 * Read strdup vaiable
 * @param var variable object
 * @return value char pointer 
 */
char *timevar_readcp(struct timevar *var);

/**
 * Test if the variable has been updated since last read
 * @param var variable object
 * @return 1 if updated or 0 if not. 
 */
int timevar_updated(struct timevar *var);


#endif /* TIMEVAR_H_ */
