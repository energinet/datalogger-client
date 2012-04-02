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

#ifndef TYPE_LOADER_H_
#define TYPE_LOADER_H_

#include "module_type.h"

/**
 * @defgroup module_type_load Module type loader
 * @{
 */

/**
 * Modult type file suffix
 */
#define FILE_SUFF ".so"

/**
 * Load a contdaem module type
 * @param path The file to load
 * @param types The list of module types
 * @return zero on success
 */
int module_type_load_file(const char *path, struct module_type **types);


/**
 * Load all contdaem module types in a directory
 * @param dir The directory to load 
 * @param types The list of module types
 * @return zero on success
 */
int module_type_load_all(const char *dir, struct module_type **types);


/**
 * Unload all module types in the list
 * @param types The list of module types
 */
void module_type_unload_file(struct module_type *types);

/**
 *@} 
 */



#endif /* MODULE_LOADER */
