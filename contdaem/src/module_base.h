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

#ifndef MODULE_BASE_H_
#define MODULE_BASE_H_

#include <pthread.h>
#include "module_type.h"
#include "module_event.h"
#include "uni_data.h"
#include "module_util.h"
#include "module_tick.h"

/**
 * @defgroup module_base_grp base module
 * @{
 */


extern struct xml_tag base_xml_tags[];
struct module_event;
struct modules;
/**
 * Module base object 
 * Base for all modules
 */
struct module_base {
    /**
     * Module name */
    char *name;     
    /**
     * Module type
     * @note A pointer to the module type object */
    struct module_type *type;
    /**
     * Verbose flag
     * @note 0="no output" 1="verbose" */
    int verbose;
    /**
     * Event flags
     * @todo Add reference to flags */
    unsigned long flags;
    /**
     * list of events
     * @note events available for this module */
    struct event_type *event_types; 
    /**
     * list of handlers open to other modules */
    struct event_handler *event_handlers; 
    /**
     * The nex module in the list */
    struct module_base *next; 
    /**
     * The first module in the list */
    struct module_base *first; 
    /**
     * run flag 
     * @note For loop thread 0=stop 1=run */  
    int run;
    /**
     * pthread object for the loop thread */  
    pthread_t loop_thread;   
    /**
     * A pointer to the tick master object */
    struct module_tick_master *tick_master;
};


/**
 * Create a new module 
 * @param type is a pointer to the module type
 * @param name is the unique name of the mosule
 * @return a pointer to the new mosule if created of NULL if fault
 * @private
 */
struct module_base* module_base_create(struct modules *modules,struct module_type* type, const char *name, const char** attr);

/**
 * Add a module to the module list
 * @param first is a pointer to the first module
 * @param new is a pointer the new module
 * @private
 */
struct module_base* module_base_list_add(struct module_base* first, struct module_base* new);


/**
 * Delete a base module and list
 * @param module a pointer to the module object 
 * @private
 */
void module_base_delete(struct module_base* module);


/**
 * Remove a module from the module list
 * @param first The first module in the list
 * @param rem The module to be removed
 * @private
 */
struct module_base* module_base_list_remove(struct module_base* first, struct module_base* rem);

/**
 * Get a module by unique name 
 * @param first is the first module in the list
 * @param name is the unique name 
 * @return a pointer to the module if found or NULL if fault
 */
struct module_base* module_base_get_by_name(struct module_base* first,const char *name);


/**
 * Subscribe to a single event
 */
int module_base_subscribe(struct module_base* subscriber, struct event_type *event, struct event_handler *handler);

/**
 * Subscribe to an event 
 * @param subscriber The subscriber module base
 * @param module_list The list of all modules
 * @param name The name of the event to search
 * @todo Describe event name \<module>.\<event>
 * @param mask The flag mask
 * @todo Describe module mask 
 * @param handler The handler to receive the event
 * @param attr The XML attributes
 * @return ?
 * @todo Check return value  
 */
int module_base_subscribe_event(struct module_base* subscriber, struct module_base* module_list, 
				const char *name, unsigned long mask,
                                struct event_handler *handler, const char **attr);

/**
 * Send an event to all listening handlers
 * @param event The event to be send
 * @return ?
 * @todo Check return value
 */
int module_base_send_event(struct module_event *event);


/**
 * Get an event of the name, <ename>
 * @param module_list The list of all modules
 * @param name The search name
 * @param mask The search event flag mask
 * @return the first match or NULL of not found 
 */
struct event_type *module_base_get_event(struct module_base* module_list,
					 const char *name, unsigned long mask);

/**
 * Read all events of \c name
 * @param module_list The list of all modules
 * @param name The search name
 * @param mask The search event flag mask
 * @return a list of events or NULL if not found or error
 */
struct module_event *module_base_read_all(struct module_base* module_list, 
					  const char *name, unsigned long mask );


/**
 * @} 
 */


/**
 * @defgroup load_list Module load list
 */

/**
 * Load or unload list (MNU!)
 */
struct load_list{
    char *name;
    int unload;
    struct load_list *next;
};

/**
 * Create or add an element to the load list
 */
struct load_list *load_list_add(struct load_list *list, char const *name);

/**
 * Check a name from in the load list
 * @return 0 no load, 1 do load
 */
int load_list_check(struct load_list *list, char const *name, char const *xmlarg);

/**
 * @} 
 */

struct modules{
    struct module_type *types;
    struct module_base *list;
    struct load_list *loadlist;
    struct module_tick_master *tick_master;
    int verbose;
};

#endif /* MODULE_BASE_H_ */
