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
#include "xml_attr.h"


#include "xml_doc.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct xml_attr *xml_attr_create(struct xml_doc *doc, const char *name, const char *value )
{
    struct xml_attr *attr = xml_doc_alloc(doc, sizeof(*attr));
    
	if(!attr){
		fprintf(stderr, "ERROR: %s is returning NULL\n",__FUNCTION__);
		return NULL;
    }
    
	attr->name = xml_doc_text_dup(doc,name,0);
    attr->value = xml_doc_text_dup(doc,value,0);
    
    return attr;
}


struct xml_attr *xml_attr_create_copy(struct xml_doc *doc, struct xml_attr *src )
{
	if(!src)
		return NULL;

	
	struct xml_attr *attr = xml_attr_create(doc, src->name, src->value);

	if(!attr)
		return NULL;

	attr->next = xml_attr_create_copy(doc, src->next);

	return attr; 
}



void xml_attr_delete(struct xml_doc *doc, struct xml_attr *attr)
{
	if(doc)
		return;
	if(!attr)
		return;

	xml_attr_delete(doc, attr->next);

	free(attr->name);
    free(attr->value);
	free(attr);
}



int xml_attr_print(struct xml_attr *attr, char *buf, size_t maxsize )
{
    int len = 0;
    
    if(!attr)
	return len;

    if(attr->value)
		len += snprintf(buf+len, maxsize-len , " %s=\"%s\"", attr->name, attr->value);
    else 
		len += snprintf(buf+len, maxsize-len , " %s", attr->name);
	
    len += xml_attr_print(attr->next, buf+len, maxsize-len );
	
    return len;
}

