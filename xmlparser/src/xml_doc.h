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
 
#ifndef XML_DOC_H_
#define XML_DOC_H_

#include <stddef.h>

#define DBGLEV_ALLOC 16

struct xml_item;

struct xml_doc {
    void *doc;
    struct xml_item *first;
    size_t used;
    size_t alloced;
    int dbglev;
};


/**
 * Create a XML seaializer document 
 */
struct xml_doc  *xml_doc_create(size_t size, int dbglev);

/**
 * Delete the document
 */
void xml_doc_delete(struct xml_doc  *doc);

/**
 * Reset document 
 */
void xml_doc_reset(struct xml_doc  *doc);


/**
 * Number of bytes remaing in document 
 */
size_t xml_doc_space_remain(struct xml_doc  *doc);

/**
 * Checks if the document har the first item (the document root)
 */
int xml_doc_hasroot(struct xml_doc  *doc);

/**
 * Checks if the root document has items 
 */
int xml_doc_roothasitems(struct xml_doc  *doc);

/**
 * Alloc a peace of memory in the document
 */
void* xml_doc_alloc(struct xml_doc  *doc, size_t size);

/**
 * Print the XML
 */
int xml_doc_print(struct xml_doc  *doc, char* buf, size_t maxsize);

/**
 * Print the stdout
 */
 void xml_doc_printf(struct xml_doc  *doc); 


/**
 * Duplicate text string */
char *xml_doc_text_dup(struct xml_doc  *doc, const char* text, int txtlen);

/**
 * Add To text string */
char *xml_doc_text_add(struct xml_doc  *doc, const char *itemtext, const char* text, int txtlen);

/**
 * Print document */
const char *xml_doc_text_printf(struct xml_doc  *doc, const char* format, ...);


#endif /* XML_DOC_H_ */
