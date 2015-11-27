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
 
#ifndef XML_ATTR_H_
#define XML_ATTR_H_

#include <stddef.h>

struct xml_doc;

struct xml_attr {
    char *name;
    char *value;
    struct xml_attr* next;
};


struct xml_attr *xml_attr_create(struct xml_doc *doc, const char *name, const char *value );

struct xml_attr *xml_attr_create_copy(struct xml_doc *doc, struct xml_attr *src );

void xml_attr_delete(struct xml_doc *doc, struct xml_attr *attr);

int xml_attr_print(struct xml_attr *attr, char *buf, size_t maxsize );



#endif /* XML_ATTR_H_ */




