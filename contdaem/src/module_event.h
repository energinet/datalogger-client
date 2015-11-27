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

#ifndef EVENT_H_
#define EVENT_H_

#include "uni_data.h"
#include "contdaem.h"
#include "module_base.h"


extern struct module_base *base;
struct event_handler;

/**
 * @defgroup module_event_grp Module event
 * @{
 */

/**
 * @defgroup module_event_flags Event flags
 * @{
 */

/**
 * It is available */
#define FLAG_AVAI  0x1 
/**
 * Show value in live view and on graph as default 
 * @note if not set this value will be hidden from user*/
#define FLAG_SHOW  0x2 
/**
 * Log this value 
 * @note require subscription from */
#define FLAG_LOG   0x4 
/**
 * Fault event */
#define FLAG_FEVT  0x8
/**
 * Event flag??
 * @todo Get definition */
#define FLAG_EVT   0x16
/**
 * Measurment 
 * @todo Get definition */
#define FLAG_MEAS  0x32
/**
 * All flags
 * @note Used for search */
#define FLAG_ALL 0xffffffff
/**
 * Default flags 
 * @note DEfault flags for a measurment */
#define DEFAULT_FLAGS (FLAG_AVAI|FLAG_SHOW|FLAG_LOG|FLAG_MEAS)
/**
 * Fault event flags
 * @note Default flags for a fault event */
#define DEFAULT_FFEVT (FLAG_AVAI|FLAG_LOG|FLAG_FEVT)

/**
 * Evalue event flags */
#define DEFAULT_FEVAL (FLAG_AVAI)


/**
 * @}
 */

/**
 * @defgroup module_event_type event type
 * @{
 */

/**
 * Event type struct 
 * 
 */
struct event_type{
    /**
     * Name of the type */
    char *name;
    /**
     * Unit name */
    char *unit;
    /**
     * Human name 
     * @note Text shown to the user*/
    char *hname;    
    /**
     * Event flags 
     * @ref module_event_flags*/
    unsigned long flags;
    /**
     * Read callback
     * @note Function for reading the current value for this event. Ignored if NULL */
    struct module_event* (*read)(struct event_type *etype);
    /**
     * Write callback
     * @note Function for writing a value to the event Ignored if NULL
     * @note Not used. Future feature */
    int (*write)(struct event_type *event, struct uni_data *data);
    /**
     * Base module 
     * @ref module_base */
    struct module_base *base;
    /**
     * Userdata for this event type 
     * @note Could be an internal object in the module*/
    void  *objdata;
    /**
     * Flow to level unit 
     * @todo Describe and reference */
    struct convunit *flunit;
    /**
     * A list of subscribers for this event type */
    struct callback_list *subscribers;
    /**
     * The next event type in this list 
     * @note The last element has next=NULL*/
    struct event_type *next;
};

/**
 * @}
 */

/**
 * @defgroup module_event_event event
 * @{
 */

/**
 * Module event object 
 * For sending events from one module to module
 */
struct module_event{
    struct module_base *source;
    struct event_type *type;
    struct uni_data *data;
    struct timeval time;
    struct module_event *next;
};


/**
 * @}
 */

/**
 * @defgroup module_event_handler Event handler
 * @{
 */

#define EVENT_HANDLER_PAR struct event_handler *handler, struct module_event *event

struct event_handler{
    char *name;
    int (*function)(EVENT_HANDLER_PAR);
    struct module_base *base;
    void *objdata;    
    struct event_handler *next;
};

/**
 * @}
 */

/**
 * @defgroup module_event_callback Event callback list
 * @{
 */

/**
 * Callback list 
 * A list of handlers 
 */
struct callback_list{
    /**
     * Handler */
    struct event_handler *handler;
    /**
     * Next handler */
    struct callback_list *next;
};
/**
 * @}
 */

/**
 * Get event flags from a string 
 * @ingroup module_event_flags
 * @param flagstr String to parse 
 * @param def_flags Default flags
 * @return read flags and/or def_flags
 */
unsigned long event_type_get_flags(const char *flagstr, unsigned long def_flags);


/**
 * Get event flags as string 
 * @ingroup module_event_flags
 * @param flags flags to write
 * @param flagstr output buffer
 * @return output buffer (flagstr)
 */
const char *event_type_get_flag_str(unsigned long flags, char *flagstr);

/**
 * @ingroup module_event_type
 * @{
 */
/**
 * Create a event type (paramaters)
 * @param base  Base module 
 * @param objdata Object data 
 * @param name Event name 
 * @param unit Unit name 
 * @param hname Human name 
 * @param flags Event flags
 * @return The new object or NULL if fault 
 */
struct event_type *event_type_create(struct module_base *base, void *objdata,
				     const char *name, const char *unit, const char *hname,
				     unsigned long flags);

/**
 * Create a event type (XML attributes)
 * @param base  Base module 
 * @param objdata Object data 
 * @param attr XML attributes
 * @return The new object or NULL if fault 
 */
struct event_type *event_type_create_attr(struct module_base *base, void *objdata, 
					  const char **attr);



/**
 * Copy a event type
 * @param base  Base module 
 * @param objdata Object data 
 * @param flags Event flags
 * @param source The source module
 * @note The base, flags and objdata from the source are not copyed
 * @return The new object or NULL if fault 
 */
struct event_type *event_type_create_copy(struct module_base *base, void *objdata, unsigned long flags, struct event_type *source);
struct event_type *event_type_create_copypart(struct module_base *base, void *obj, const char **attr, struct event_type *source);

/**
 * Add an event type to a list
 * @param first The first element in the list
 * @param new The new element to add
 * @return The new first element in the list 
 * @todo Describe the "standard" linked list with exampel */ 
struct event_type *event_type_add(struct event_type *first, struct event_type *new);

/**
 * Delete a event type/list
 * @param etypes A event type or a list of types 
 */
void event_type_delete(struct event_type *etypes);





/**
 * Get event by name 
 * @param list The list to search 
 * @param name The name to search for
 * @return The first module meets the search criteria or NULL if not found or error
 */
struct event_type *event_type_get_by_name(struct event_type *list, const char *name);

/**
 * Count the number of subscribers 
 * @param etype The ebent type
 * @return the count 
 * @note return zero of etype is NULL
 */
int event_type_get_subscount(struct event_type *etype);



/**
 * Read event value 
 * @param etype The event type
 * @return valid event or NULL if error
 */
struct module_event *event_type_read(struct event_type *etype);

/**
 * Write event value 
 */
int event_type_write( struct event_type *event, struct uni_data *data);

/**
 * @}
 */

/**
 * @ingroup module_event_handler
 * @{
 */

struct event_handler *event_handler_create(const char *name,  int (*function)(EVENT_HANDLER_PAR), struct module_base *base, void *objdata);

struct event_handler *event_handler_list_add(struct event_handler *list , struct event_handler *new);
void event_handler_delete(struct event_handler *handler);


struct event_handler * event_handler_get(struct event_handler *handler_list, const char *name);

/**
 * @}
 */

/**
 * @ingroup module_event_callback
 * @{
 */

struct callback_list* callback_list_create(struct event_handler *handler);

struct callback_list* callback_list_add(struct callback_list* first, struct callback_list* new);

void callback_list_delete(struct callback_list* call);

int callback_list_run(struct callback_list* list, struct module_event* event);
/**
 * @}
 */

struct module_event* module_event_create(struct event_type *type, 
                                         struct uni_data *data,  struct timeval *time);

struct module_event* module_event_copy(struct module_event* source);
struct module_event* module_event_add(struct  module_event* list, struct  module_event* new);


int module_event_get_txt_time( struct module_event *event, char* buffer, int max_size);

void module_event_delete(struct module_event* rem);


int module_event_get_txt_name(struct module_event *event, char* buffer, int max_size);

int  module_event_sprint(struct  module_event* event, char *buf, int maxlen);




struct event_search{
    char name[64];
    char *m_name;
    char *e_name;
    struct module_base  *m_curr;
    struct event_type *e_next;
    unsigned long mask;
    int dbglev;
};


void event_search_init(struct event_search* search, struct module_base* module_list, const char *name, unsigned long mask);
 
struct event_type *event_search_next(struct event_search* search);



/**
 * @}
 */

#endif /* EVENT_H_ */
