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
 
#ifndef MODULE_TYPE_H_
#define MODULE_TYPE_H_

#include <xml_parser.h>
#include "module_event.h"

/**
 * @defgroup module_type_grp module type
 * @{
 */


/**
 * Module type
 * @deprecated Not used
 * @memberof module_type
 * @todo Remove from alle files
 */

enum module_types{
    MTYPE_INPUT,
    MTYPE_OUTPUT,
    MTYPE_CONTROL,
    MTYPE_DATALOG,
};


/**
 * Module subtype
 * @deprecated Not used
 * @memberof module_type
 * @todo Remove from alle files
 */
enum module_subtypes{
    SUBTYPE_FLOW,
    SUBTYPE_LEVEL,
    SUBTYPE_EVENT,
    SUBTYPE_NONE,
};


struct module_base;
struct event_type;

/**
 * Module type 
 * Detaild description
 */
struct module_type {
    /**
     * @brief Name of type */
    char *name;
    /**
     * @brief Type 
     * @deprecated Not used
     * @todo Remove   */
    enum module_types mtype;
    /**
     * @brief Subtype 
     * @deprecated Not used
     * @todo Remove   */
    enum module_subtypes subtype;
    /**
     * Init callback.
     * This callback is called when the module is created during the parsing of the XML configuration file.
     * A NULL pointer will be ignored
     * @param base The @ref module_base
     * @param attr The XML attributes 
     * @return zero on success or negative value if error */     
    int (*init)(struct module_base *base, const char **attr);
    /**
     * Start callback.
     * This callback is called when the XML configuration has been passed and all 
     * modules has been initialized.
     * @param base The @ref module_base
     * @return zero on success or negative value if error */
    int (*start)(struct module_base *base);
    /**
     * Loop callback. Module thread 
     * This will be called when the modules has been started 
     * It should contain a while(@ref module_type::run) loop
     * @param base A pointer to the @ref module_base */
    void* (*loop)(void *base);
    /**
     * Deinit callback.
     * This will be called when the module are going to be unloaded. 
     * It can be used to close file descriptors 
     * @param base The @ref module_base */
    void (*deinit)(struct module_base *base);
    /**
     * Subscribe callback
     * Used for advanced subscribing 
     * @param base The @ref module_base
     * @param source The @ref module_base of the source module 
     * @param event_type The event type
     * @param attr the XML attributes from the XML item initiating the subscribe*/
    struct event_handler *(*subscribe)(struct module_base *base, struct module_base *source, 
				       struct event_type *event_type, const char **attr);
    /**
     * Struct size.
     * The size of the module struct for this module type */
    int type_struct_size;   
    /**
     * Type specific XML tags 
     * See libxmlparser for more information */
    struct xml_tag *xml_tags;
    /**
     * Handler list 
     * List of handlers to preload */
    struct event_handler *handlers;
    /**
     * dynamic loader handel.
     * Is used to unload the module type, when the application is closed 
     * @private */
    void* dlopen_handel;
    /**
     * The next type in the list */
    struct module_type *next;
};


/**
 * Add a module type to a list
 * @param first The first element of the list
 * @param new The element to add
 * @return The new first element of the list 
 * @memberof module_type
 */
struct module_type *module_type_list_add(struct module_type *first, struct module_type *new);



/**
 * Get a module type by name
 * @param types A list of module types
 * @param name The name to get
 * @memberof module_type
 * @return Ti found the @ref module_type else NULL
 */
struct module_type *module_type_get_by_name(struct module_type *types, const char* name);


/**
 * Print a @ref module_type
 * @param type A module type
 * @memberof module_type
 */
void module_type_print(struct module_type *type);

/**
 * Print a list of @ref module_type
 * @param types Module type list
 * @memberof module_type
 */
void module_type_list_print(struct module_type *types);


/**
 * Delete @ref module_type
 * @param types A module type or a module type list
 * @note recursive
 * @memberof module_type
 */
void module_type_delete(struct module_type *types);


/**
 * @}
 */



#endif /* MODULE_TYPE_H_ */
