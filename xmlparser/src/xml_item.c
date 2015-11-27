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
#include "xml_item.h"

#include "xml_doc.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>



struct xml_item *xml_item_create(struct xml_doc *doc, const char *name)
{
    struct xml_item *item = xml_doc_alloc(doc, sizeof(*item));
    
    if(!name){
		fprintf(stderr, "ERROR: in %s : no name (NULL)\n",__FUNCTION__);
		return NULL;
    }

    if(!item){
		fprintf(stderr, "ERROR: %s is returning NULL\n",__FUNCTION__);
		return NULL;
    }

    if(doc) if(doc->dbglev>=DBGLEV_ITEM)
				fprintf(stderr, "create item %s\n", name);

    item->name = xml_doc_text_dup(doc, name, 0);
	item->doc  = doc;

    return item;
}



struct xml_item *xml_item_create_copy(struct xml_doc *doc, struct xml_item *src, enum item_copy_type cpynext)
{
    struct xml_item *item;
    
    if(!src){
		return NULL;
    }

    if(doc) if(doc->dbglev>=DBGLEV_ITEM)
		fprintf(stderr, "copying item %s\n", src->name);
	
	item = xml_item_create(doc, src->name);

	if(!item){
		return NULL;
    }
	
	item->attrs = xml_attr_create_copy(doc, src->attrs);
	item->text  = xml_doc_text_dup(doc, src->text, 0);
	item->childs = xml_item_create_copy(doc, src->childs, ICOPY_ALL);
	if(cpynext == ICOPY_ALL)
	    item->next  = xml_item_create_copy(doc, src->next, ICOPY_ALL);
	else
	    item->next  = NULL;

    return item;
}


struct xml_item *xml_item_list_add(struct xml_item *list, struct xml_item *new)
{
    struct xml_item *ptr = list;

    if(!ptr){
	return new;
    }
      
    while(ptr->next)
		ptr = ptr->next;
    
    ptr->next = new;
    
    return list;
    
}




void xml_item_add_text(struct xml_doc *doc, struct xml_item *item, const char *text, int txtlen)
{
    if(!item->childs||!doc){
		item->text = xml_doc_text_add(doc, item->text, text, txtlen);
    }
	
}

void xml_item_add_attr(struct xml_doc *doc, struct xml_item *item, const char *name, const char *value)
{
    struct xml_attr *ptr   = item->attrs;
    struct xml_attr *attr = xml_attr_create(doc, name, value);

    if(!attr){
		fprintf(stderr, "ERROR: in %s : attr is NULL\n",__FUNCTION__);
		return;
    }

    if(doc) if(doc->dbglev>=DBGLEV_ITEM){
			fprintf(stderr, "%p=\"%p\"\n", attr->name, attr->value);
			fprintf(stderr, "%s=\"%s\"\n", attr->name, attr->value);
		}

    if(!ptr){
        item->attrs = attr;
        return;
    }

    while(ptr->next)
        ptr = ptr->next;

    ptr->next = attr;
    
}



void xml_item_add_next(struct xml_doc *doc, struct xml_item *item, struct xml_item *next)
{
    item->next = xml_item_list_add( item->next, next);
}

struct xml_item *xml_item_add_child(struct xml_doc *doc, struct xml_item *parent, struct xml_item *child)
{
    if(!parent){
	fprintf(stderr, "ERROR in %s : parent is NULL\n",__FUNCTION__);
	return NULL;
    }

    parent->childs = xml_item_list_add( parent->childs, child);

    return parent->childs;
    
}




void xml_item_delete(struct xml_doc *doc, struct xml_item *item)
{
	if(doc)
		return;
	if(!item)
		return;

	xml_item_delete(doc, item->next);
	xml_item_delete(doc, item->childs);
	xml_attr_delete(doc, item->attrs);
	
	free(item->name);
	free(item->text);
	free(item);
}



int xml_item_print(struct xml_item *item, char *buf, size_t maxsize )
{
    int len = 0;
    
    if(!item)
	return 0;

//  fprintf(stderr, "item %s %p %p\n", item->name, item, item->name);

    len += snprintf(buf+len, maxsize-len, "<%s", item->name);

    len += xml_attr_print(item->attrs, buf+len, maxsize-len );

    if(!item->text && !item->childs){
        len += snprintf(buf+len, maxsize-len, "/>");
    } else {
        len += snprintf(buf+len, maxsize-len, ">");

        if(item->text)
	    len += snprintf(buf+len, maxsize-len,"%s", item->text);   

	len += xml_item_print( item->childs, buf+len,  maxsize-len );

	len += snprintf(buf+len, maxsize-len,"</%s>",  item->name);
    }

    
    len += xml_item_print( item->next, buf+len,  maxsize-len );

    return len;

}

/**
 * Get a item tag name
 * @param item The XML-item
 * @return The text or NULL if none
 */
const char *xml_item_get_tag(struct xml_item *item)
{
    return item->name;
}


const char *xml_item_get_attr(struct xml_item *item, const char *name)
{
    struct xml_attr *ptr = item->attrs;
    assert(name);
    
    while(ptr){
	if(strcmp(name, ptr->name)==0)
	    return ptr->value;
	ptr = ptr->next;
    }

    return NULL;
    
}


const char *xml_item_get_text(struct xml_item *item)
{
    return item->text;
}



void xml_item_printf(struct xml_item *item)
{
	char buf[100000];
	
	xml_item_print(item, buf, sizeof(buf));

	printf("%s\n",  buf);
	
}





struct xml_item *xml_item_get_child(struct xml_item *item, struct xml_item *prevchi, const char *tag)
{
    struct xml_item *ptr = item->childs;

    assert(tag);

    if(prevchi)
		ptr = prevchi->next;;

    while(ptr){
		if(strcmp(tag, ptr->name)==0)
			return ptr;
		ptr = ptr->next;
    }
    
    return NULL;
}

