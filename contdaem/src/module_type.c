/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2011 LIAB ApS <info@liab.dk>
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

#include "module_type.h"
#include "contdaem.h"
#include <dirent.h> 
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>

struct module_type *module_type_list_add(struct module_type *first,
		struct module_type *new) {

	struct module_type *ptr = first;

	if (!first)
		/* first module in the list */
		return new;

	/* find the last module in the list */
	while (ptr->next) {
		ptr = ptr->next;
	}

	ptr->next = new;

	return first;

}

struct module_type *module_type_get_by_name(struct module_type *types,
		const char* name) {
	struct module_type *ptr = types;

	while (ptr) {
		if (strcmp(ptr->name, name) == 0)
			return ptr;
		ptr = ptr->next;
	}

	return NULL;

}

void module_type_print(struct module_type *type) {

	printf("Module type %s\n", type->name);

}

void module_type_list_print(struct module_type *type) {
	struct module_type *ptr = type;
	while (ptr) {
		module_type_print(ptr);
		ptr = ptr->next;
	}

}

void module_type_delete(struct module_type *type) {
	if (type->next)
		module_type_delete(type->next);
}
