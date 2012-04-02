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
 
#ifndef EVALUE_H_
#define EVALUE_H_

#include <pthread.h>
#include "module_event.h"


#define EVAL_CALL_PAR struct evalue* evalue

/**
 * @section evaule Evalue (Event Value)
 * A event value is an ovject containing a value with all the methods 
 * implemented for subscribeing, sending, reading and writing.
 * 
 * @subsection evalueparam Evalue parameters
 * When creating the evalue object a parameter can be parsed to the object creator function.
 * 
 */

/**
 * evalue struct
 * En event value object containing subscribe, submit event , read and write.
 */
struct evalue{
    /**
     * Value @private */
    float value;
    /**
     * Update time */
    struct timeval time;
    /**
     * Name of the value */
    char *name;
    /**
     * Event type */
    struct event_type *event_type;
    /**
     * Object mutex */
    pthread_mutex_t mutex;
    /**
     * Callback function for when the value is changed */
    void (*callback)(EVAL_CALL_PAR);
    /**
     * Userdata pointer for the callback function */
    void *userdata;
    /**
     * A pointer to the base module object */
    struct module_base *base;
};

/**
 * Create evalue object using tage attributes 
 * @param base Pointer to the module base object
 * @param param Module value parameters 
 * @param attr Tag attributes list
 * @return A evalue object or NULL on error
*/
struct evalue* evalue_create_attr(struct module_base *base, const char *param, 
				  const char **attr);

/**
 * Create evalue object
 * @param base Pointer to the module base object
 * @param name The name of the event type
 * @param subname The subname of the event type (will become <name>_<subname>)
 * @param param Module value parameters 
 * @param flags Event type flags
 * @param attr Tag attributes list
 * @return A evalue object or NULL on error
*/
struct evalue* evalue_create(struct module_base *base, const char *name,  const char *subname, const char *param, unsigned long flags);

/**
 * Delete an evalue 
 * @param evalue The evalue object to delete
 */
void evalue_delete(struct evalue* evalue);

/**
 * Subscribe to an event type
 * @note If an event type name is given in the 'param' parameter when creating an event type, 
 *       then the event type will be subscribed 
 * @note This will sunscribe to the first matching event type 
 * @param evalue The evalue object
 * @param event_name The event type name
 * @param mask The event type mask
 * @return zero or positive value on success og negative value on error 
*/
int evalue_subscribe(struct evalue* evalue, const char *event_name, unsigned long  mask);

/**
 * Get float value 
 * @param evalue The evalue object */
float evalue_getf(struct evalue* evalue);

const char *evalue_unit(struct evalue* evalue);

/**
 * get uni_data value 
 * @param evalue The evalue object */
struct uni_data *evalue_getdata(struct evalue* evalue);

/**
 * Set float value with time
 * @param evalue The evalue object
 * @param value The set value 
 * @param now A pointer to a timeval object
 */
void evalue_setft(struct evalue* evalue, float value, struct timeval *now);

/**
 * Set the value with time
 * @param evalue The evalue object
 * @param value The set value 
 * @param now A pointer to a timeval object
 */
void evalue_setf(struct evalue* evalue, float value);

/**
 * Check if valid data has been written 
 * @return 0 if invalid and 1 if valid  */
int evalue_isvalid(struct evalue* evalue);

/**
 * Set the data with time=now
 * @param evalue The evalue object
 * @param data The set data
 */
void evalue_setdata(struct evalue* evalue, struct uni_data *data, struct timeval *now);

/**
 * Register a callback function 
 * @param evalue The evalue object 
 * @param callback The callback function
 * @param userdate The userdate pointer for the callback
 */
void evalue_callback_set(struct evalue* evalue, void (*callback)(EVAL_CALL_PAR), void* userdata);

/**
 * Remove the callback function 
 * @param evalue The evalue object
 */
void evalue_callback_clear(struct evalue* evalue);


/**
 * Check if two evalue are in sync; have the same update time 
 * @note It is important that the time tick functionality is used
 * @param evalueA The 1st evalue object 
 * @param evalueB The 2nd evalue object 
 * @param valueA A pointer where the value of A is returned
 * @param valueB A pointer where the value of B is returned
 * @param stime A pointer where the sync time is returned
 * @return 1 if A and B are in sync of if not 
 */
int evalue_insync(struct evalue* evalueA, struct evalue* evalueB, 
		  float *valueA, float *valueB, struct timeval *stime);

#endif /* EVALUE_H_ */
