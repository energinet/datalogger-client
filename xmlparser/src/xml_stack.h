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
 
#ifndef XML_STACK_H_
#define XML_STACK_H_

#include <expat.h>
#include <stddef.h>

struct xml_doc;
struct xml_item;

struct xml_stack {
    XML_Parser parser ;
    struct xml_doc  *doc;
    struct xml_item **stack;
    int depth;
};


/**
 * Create stack object 
 */
struct xml_stack  *xml_stack_create(size_t stacksize, struct xml_doc  *doc, int dbglev);


/**
 * Delete stack object 
 */
void xml_stack_delete(struct xml_stack  *stack);

/**
 * Reset stack object 
 */
void xml_stack_reset(struct xml_stack  *stack);

/**
 * Push item onto stack element
 */
struct xml_item *xml_stack_push(struct xml_stack *stack, struct xml_item *item);

/**
 * Pop item from stack element
 */
void xml_stack_pop(struct xml_stack *stack);

/**
 * Get current element 
 */
struct xml_item *xml_stack_cur(struct xml_stack *stack);


/**
 * Parse a step
 */
int xml_stack_parse_step(struct xml_stack *stack, char *buf, int size);


#endif /* XML_STACK_H_ */
