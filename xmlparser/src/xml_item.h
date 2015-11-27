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
 
#ifndef XML_ITEM_H_
#define XML_ITEM_H_

#define DBGLEV_ITEM 8

#include "xml_attr.h"


struct xml_doc;

struct xml_item {
	/**
	 * Tag name */
    char *name;
	/**
	 * Text */
	char *text;
	/**
	 * Attribute list */
    struct xml_attr *attrs;
	/**
	 * Child list */
    struct xml_item *childs;
	/**
	 * Next item in list  */	
    struct xml_item *next;
	struct xml_doc *doc;
};

enum item_copy_type {
  ICOPY_SINGLE = 0,
  ICOPY_ALL = 1,
};


/**
 * Create a XML-item
 * @param doc The XML-document object, or NULL if no document
 * @param tag The xml tage name
 * @return A pointer to the XML-item
 */
struct xml_item *xml_item_create(struct xml_doc *doc, const char *tag);

/**
 * Copy a XML-item
 * @param doc The XML-document object, or NULL if no document
 * @param src The source object
 * @return A pointer to the XML-item
 */
struct xml_item *xml_item_create_copy(struct xml_doc *doc, struct xml_item *src, enum item_copy_type cpynext);

/**
 * Delete a XML-item
 * @param doc The XML-document object, or NULL if no document
 * @param item The XML-item to delete
 * @note This will not delete the object if it is included inn a doc
 * @note This will delete all childs, attributes and next in list
 */
void xml_item_delete(struct xml_doc *doc, struct xml_item *item);


struct xml_item *xml_item_list_add(struct xml_item *list, struct xml_item *new);


/**
 * Add text to a XML-item
 * @param doc The XML-document object, or NULL if no document
 * @param item The XML-item
 * @param text The text to add 
 * @param txtlen The length of the text
 */
void xml_item_add_text(struct xml_doc *doc, struct xml_item *item, const char *text, int txtlen);

/**
 * Add a attribut to a XML-item
 * @param doc The XML-document object, or NULL if no document
 * @param item The XML-item
 * @param text The attribute name 
 * @param text The attribute value
 */
void xml_item_add_attr(struct xml_doc *doc, struct xml_item *item, const char *name, const char *value);

/**
 * Add a XML-item after a XML-item
 * @param doc The XML-document object, or NULL if no document
 * @param item The XML-item to modify
 * @param next The XML-item to add 
 */
void xml_item_add_next(struct xml_doc *doc, struct xml_item *item, struct xml_item *next);

/**
 * Add a XML-item child to a XML-item
 * @param doc The XML-document object, or NULL if no document
 * @param parent The XML-item to modyfi
 * @param child The XML-item to add 
 */
struct xml_item *xml_item_add_child(struct xml_doc *doc, struct xml_item *parent, struct xml_item *child);


/**
 * Get a xml child by tag
 * @param item The XML-item
 * @param prevchi The previous child or NULL of none
 * @param tag The tage to search for 
 * @return The found child or NULL if not found
 */
struct xml_item *xml_item_get_child(struct xml_item *item, struct xml_item *prevchi, const char *tag);


/**
 * Get a item tag name
 * @param item The XML-item
 * @return The text or NULL if none
 */
const char *xml_item_get_tag(struct xml_item *item);


/**
 * Get a attribute by name
 * @param item The XML-item
 * @param name The name to search for 
 * @return The attribute value or NULL it not found
 */
const char *xml_item_get_attr(struct xml_item *item, const char *name);

/**
 * Get a item text
 * @param item The XML-item
 * @return The text or NULL if none
 */
const char *xml_item_get_text(struct xml_item *item);

/**
 * Print the XML-item to a buffer
 * @param item The XML-item
 */
int xml_item_print(struct xml_item *item, char *buf, size_t maxsize );

/**
 * Print the XML-item to screen
 * @param item The XML-item
 */
void xml_item_printf(struct xml_item *item);






#endif /* XML_ITEM_H_ */




