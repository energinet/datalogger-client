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

#include "xml_serialize.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <expat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>

struct xml_stack  *xml_stack_create(size_t stacksize, size_t docsize, int dbglev)
{
    struct xml_stack  *stack = malloc(sizeof(*stack));
    assert(stack);

    stack->stack = malloc(sizeof(struct xml_stack*)*stacksize);
    stack->depth = -1;

    stack->doc = xml_doc_create(docsize, dbglev);

    return stack;

}


void xml_stack_delete(struct xml_stack *stack)
{
    if(!stack)
	return;

    free(stack->stack);
    xml_doc_delete(stack->doc);
    free(stack);
}



void xml_stack_reset(struct xml_stack  *stack)
{
    stack->depth = -1;
    xml_doc_reset(stack->doc);
}

struct xml_item *xml_stack_push(struct xml_stack *stack, struct xml_item *item)
{
    struct xml_item *parent =NULL;
    
    if(stack->depth>=0)
	parent = stack->stack[stack->depth];
    
    stack->stack[++stack->depth] = item;

    return parent;
}

void xml_stack_pop(struct xml_stack *stack)
{


    if(stack->depth<0){
	fprintf(stderr, "under vandet\n");
	return;
    }

    stack->depth--;
}


struct xml_item *xml_stack_cur(struct xml_stack *stack)
{
    return stack->stack[stack->depth];
}




void xml_doc_reset(struct xml_doc  *doc)
{
    doc->first = NULL;
    doc->used = 0;
}


int xml_doc_space_remain(struct xml_doc  *doc)
{
    return doc->alloced - doc->used;
}


struct xml_doc  *xml_doc_create(size_t size, int dbglev)
{
    struct xml_doc  *doc = malloc(sizeof(*doc));
    assert(doc);

    xml_doc_reset(doc);
    doc->doc =  malloc(size);
    assert(doc->doc);
    xml_doc_reset(doc);
    doc->alloced = size;
    doc->dbglev = dbglev;

    return doc;
}

void xml_doc_delete(struct xml_doc  *doc)
{
    if(!doc)
	return;

    free(doc->doc);

    free(doc);
}

void* xml_doc_alloc(struct xml_doc  *doc, size_t size)
{
    void *ptr = doc->doc + doc->used;
    size_t size4 = size;

    /* 32 bit align */
    if(size4%4)
	size4 =	((size/4)*4)+4;

    doc->used += size4;

    if(xml_doc_space_remain(doc)<doc->alloced/2){
	fprintf(stderr, "space %d %d\n", xml_doc_space_remain(doc), size4);
    }

    if(doc->used >= doc->alloced){
	fprintf(stderr, "ERROR: %s no more space (used %d, alloced size %d, alloc size %d (%d))\n"
		,__FUNCTION__, doc->used,  doc->alloced, size, size4);
	return NULL;
    }

    memset(ptr, 0, size4);
    
    if(doc->dbglev>=DBGLEV_ALLOC)
	fprintf(stderr, "alloc %d %d %d\n", size, size4, doc->used);

    return ptr;
    	
}


const char *xml_doc_text_dup(struct xml_doc  *doc, const char* text, int txtlen)
{
    size_t len = txtlen;
    char *ret  = NULL;

    if(!text){
	fprintf(stderr, "ERROR in %s: text is NULL\n",__FUNCTION__);
	return NULL;
    }
	

    if(!len)
	len = strlen(text);
    
    len++;
    ret = xml_doc_alloc(doc, len);

    if(!ret){
	fprintf(stderr, "ERROR: %s is returning NULL\n",__FUNCTION__);
	return NULL;
    }
    
    memcpy(ret, text, len);
    return ret;
}

const char *xml_doc_text_add(struct xml_doc  *doc, const char* doctext, const char* text, int txtlen)
{
    size_t len = txtlen;
    char *ret = NULL;
    
    if(!len)
	len = strlen(text);

    ret = xml_doc_alloc(doc, len+1);

    if(!ret){
	fprintf(stderr, "ERROR: %s is returning NULL\n",__FUNCTION__);
	return NULL;
    }

    if(doctext) 
	ret--; 

    memcpy(ret, text, len+1);
    ret[len] = '\0';
    
    if(!doctext)
	return ret;
    
    return doctext;   
}

const char *xml_doc_text_printf(struct xml_doc  *doc, const char* format, ...)
{ 
    char str[512];
    int len;
    va_list ap;
    char *docstr;
    
    va_start (ap, format);
    
    len = vsnprintf(str, sizeof(str), format, ap);
    
    va_end(ap);

    docstr = xml_doc_alloc(doc, len+2);
    
    if(!docstr)
	return NULL;

    memcpy(docstr, str, len+2);

    docstr[len] = '\0';

    return docstr;
} 


struct xml_attr *xml_attr_create(struct xml_doc *doc, const char *name, const char *value )
{
    struct xml_attr *attr = xml_doc_alloc(doc, sizeof(*attr));
    
    if(!attr){
	fprintf(stderr, "ERROR: %s is returning NULL\n",__FUNCTION__);
	return NULL;
    }
    
	
    attr->name = name;
    attr->value = value;
    
    return attr;
}


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

    if(doc->dbglev>=DBGLEV_ITEM)
	fprintf(stderr, "create item %s\n", name);

    item->name = name;

    return item;
}

void xml_item_add_text(struct xml_doc *doc, struct xml_item *item, const char *text, int txtlen)
{
    if(!item->childs){
	item->text = xml_doc_text_add(doc, item->text, text, txtlen);
	if(!item->text){
	    fprintf(stderr, "ERROR: in %s: adding text\n",__FUNCTION__);
	}
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

    if(doc->dbglev>=DBGLEV_ITEM){
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


struct xml_item *xml_item_add(struct xml_item *list, struct xml_item *new)
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

void xml_item_add_next(struct xml_doc *doc, struct xml_item *item, struct xml_item *next)
{
    item->next = xml_item_add( item->next, next);
}

struct xml_item *xml_item_add_child(struct xml_doc *doc, struct xml_item *parent, struct xml_item *child)
{
    if(!parent){
	fprintf(stderr, "ERROR in %s : parent is NULL\n",__FUNCTION__);
	return NULL;
    }

    parent->childs = xml_item_add( parent->childs, child);

    return parent->childs;
    
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

int xml_doc_print(struct xml_doc  *doc, char* buf, size_t maxsize)
{
    if(!doc)
	return 0;

    if(doc->used < sizeof(struct xml_item))
	return 0;

    if(doc->dbglev >= DBGLEV_ITEM )
	fprintf(stderr, "before xml_item_print\n");

    return xml_item_print(doc->first, buf,  maxsize );
}



void xml_doc_tag_start(void *data, const char *el, const char **attr)
{
    struct xml_stack *stack = (struct xml_stack *)data;
    struct xml_doc *doc = stack->doc;
    struct xml_item *cur = xml_item_create(doc, xml_doc_text_dup(doc, el,0));
    struct xml_item *par = NULL;
    int i = 0;
    
    if(!cur){
	fprintf(stderr, "ERROR: in %s : cor is NULL\n",__FUNCTION__);
	return;
    }

    par  = xml_stack_push(stack, cur);

    if(par)
	xml_item_add_child(doc, par, cur);
    else
	doc->first= cur;

    while(attr[i]){
	xml_item_add_attr(doc, cur,  
			  xml_doc_text_dup(doc, attr[i],0),
			  xml_doc_text_dup(doc, attr[i+1],0));
	i += 2;
    }

    return;
    
}




void xml_doc_char_hndl(void *data, const char *txt, int txtlen) 
{
    struct xml_stack *stack = (struct xml_stack *)data;
    struct xml_doc *doc = stack->doc;
    struct xml_item *cur  = xml_stack_cur(stack);

    xml_item_add_text(doc, cur, txt, txtlen);


}


void xml_doc_tag_end(void *data, const char *el) 
{
    struct xml_stack *stack = (struct xml_stack *)data;
    xml_stack_pop(stack);
}


void xml_doc_parse_init(XML_Parser p, struct xml_stack *stack)
{
    XML_SetUserData(p, (void *) stack);
    XML_SetElementHandler(p, xml_doc_tag_start, xml_doc_tag_end);
    XML_SetCharacterDataHandler(p, xml_doc_char_hndl); 
}

struct xml_stack * xml_doc_parse_prep(size_t stacksize, size_t docsize, int dbglev){
    
    struct xml_stack *stack = xml_stack_create(stacksize,docsize, dbglev);

    XML_Parser p = XML_ParserCreate(NULL);
    stack->parser = p;
    
    xml_doc_parse_init(p, stack);
    
    return stack;
}



int xml_doc_parse_step(struct xml_stack *stack, char *buf, int size){
    
    int retval = 0;


    if(!XML_Parse(stack->parser, buf, size, 0)) {
        fprintf(stderr, "Parse error at line %d:%s\n",
                (int)XML_GetCurrentLineNumber(stack->parser),
                XML_ErrorString(XML_GetErrorCode(stack->parser)));
        retval = -5;
    }
    
    

    if(retval<0)
	return retval;
    
    if(stack->depth == -1 && !stack->doc->first)
	return 0;
	

    return stack->depth;
}

int xml_doc_parse_free(struct xml_stack *stack)
{
   
    XML_ParserFree(stack->parser);
    xml_stack_delete(stack);

    return 0;
}




void xml_doc_parse_reset(struct xml_stack *stack)
{
    XML_Parser p = NULL;
    xml_stack_reset(stack);

    XML_ParserFree(stack->parser);
    
    p = XML_ParserCreate(NULL);
    stack->parser = p;
    xml_doc_parse_init(p, stack);
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
