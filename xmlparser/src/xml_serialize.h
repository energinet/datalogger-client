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
 
#ifndef XML_SERIALIZE_H_
#define XML_SERIALIZE_H_

#include <stdlib.h>
#include <expat.h>

#define DBGLEV_ALLOC 16
#define DBGLEV_ITEM 8

struct xml_attr {
    const char *name;
    const char *value;
    struct xml_attr* next;
};


struct xml_item {
    const char *name;
    const char *text;
    struct xml_attr *attrs;
    struct xml_item *childs;
    struct xml_item *next;
};


struct xml_doc {
    void *doc;
    struct xml_item *first;
    size_t used;
    size_t alloced;
    int dbglev;
};


struct xml_stack {
    XML_Parser parser ;
    struct xml_doc  *doc;
    struct xml_item **stack;
    int depth;
};


struct xml_stack  *xml_stack_create(size_t stacksize, size_t docsize, int dbglev);

void xml_stack_delete(struct xml_stack  *stack);

void xml_stack_reset(struct xml_stack  *stack);

struct xml_item *xml_stack_push(struct xml_stack *stack, struct xml_item *item);

void xml_stack_pop(struct xml_stack *stack);

struct xml_item *xml_stack_cur(struct xml_stack *stack);

/**
 * Create a XML seaializer document 
 */
struct xml_doc  *xml_doc_create(size_t size, int dbglev);

/**
 * Delete the document
 */
void xml_doc_delete(struct xml_doc  *doc);

void xml_doc_reset(struct xml_doc  *doc);

int xml_doc_space_remain(struct xml_doc  *doc);

/**
 * Alloc a peace of memory in the document
 */
void* xml_doc_malloc(struct xml_doc  *doc, size_t size);

/**
 * Print the XML
 */
int xml_doc_print(struct xml_doc  *doc, char* buf, size_t maxsize);


const char *xml_doc_text_dup(struct xml_doc  *doc, const char* text, int txtlen);
const char *xml_doc_text_add(struct xml_doc  *doc, const char *itemtext, const char* text, int txtlen);
const char *xml_doc_text_printf(struct xml_doc  *doc, const char* format, ...);



struct xml_attr *xml_attr_create(struct xml_doc *doc, const char *name, const char *value );


struct xml_item *xml_item_create(struct xml_doc *doc, const char *name);

void xml_item_add_text(struct xml_doc *doc, struct xml_item *item, const char *text, int txtlen);
void xml_item_add_attr(struct xml_doc *doc, struct xml_item *item, const char *name, const char *value);

void xml_item_add_next(struct xml_doc *doc, struct xml_item *item, struct xml_item *next);
struct xml_item *xml_item_add_child(struct xml_doc *doc, struct xml_item *parent, struct xml_item *child);


int xml_attr_print(struct xml_attr *attr, char *buf, size_t maxsize );
int xml_item_print(struct xml_item *item, char *buf, size_t maxsize );



struct xml_stack * xml_doc_parse_prep(size_t stacksize, size_t docsize, int dbglev);
int xml_doc_parse_step(struct xml_stack *stack, char *buf, int size);
int xml_doc_parse_free(struct xml_stack *stack);
void xml_doc_parse_reset(struct xml_stack *stack);

struct xml_item *xml_item_get_child(struct xml_item *item, struct xml_item *prevchi, const char *tag);
const char *xml_item_get_attr(struct xml_item *item, const char *name);
const char *xml_item_get_text(struct xml_item *item);


#endif /* XML_SERIALIZE_H_ */
