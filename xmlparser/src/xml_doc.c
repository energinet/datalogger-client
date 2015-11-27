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
#include "xml_doc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <assert.h>
#include "xml_item.h"
#include "xml_stack.h"


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

void xml_doc_reset(struct xml_doc  *doc)
{
    doc->first = NULL;
    doc->used = 0;
}


size_t xml_doc_space_remain(struct xml_doc  *doc)
{
    return doc->alloced - doc->used;
}


void* xml_doc_alloc(struct xml_doc  *doc, size_t size)
{
	void *ptr;
	size_t size4 = size;

	if(doc){
		ptr = doc->doc + doc->used;
		
		
		/* 32 bit align */
		if(size4%4)
			size4 =	((size/4)*4)+4;
		
		doc->used += size4;

		if(xml_doc_space_remain(doc)<doc->alloced/2){
			fprintf(stderr, "space %zd %zd\n", xml_doc_space_remain(doc), size4);
		}
		
		if(doc->used >= doc->alloced){
			fprintf(stderr, "ERROR: %s no more space (used %zd, alloced size %zd, alloc size %zd (%zd))\n"
					,__FUNCTION__, doc->used,  doc->alloced, size, size4);
			return NULL;
		}
	} else {
		ptr = malloc(size4);
	}
	
    memset(ptr, 0, size4);
    
	if(doc)	if(doc->dbglev>=DBGLEV_ALLOC)
			fprintf(stderr, "alloc %zd %zd %zd\n", size, size4, doc->used);

    return ptr;
    	
}


char *xml_doc_text_dup(struct xml_doc  *doc, const char* text, int txtlen)
{
    size_t len = txtlen;
    char *ret  = NULL;

    if(!text){
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

char *xml_doc_text_add(struct xml_doc  *doc, const char* doctext, const char* text, int txtlen)
{
    size_t len = txtlen;
    char *ret = NULL;
    
//    fprintf(stderr, "%s: doc %p, doctext %p, text %p, txtlen %d\n", __FUNCTION__, doc, doctext,  text, txtlen);
//    fprintf(stderr, "%s: doctext '%s', text '%s'\n", __FUNCTION__, doctext,  text);

    if(!len)
		len = strlen(text);

	

	if(doc){
		ret = xml_doc_alloc(doc, len+1);
		
		if(!ret){
			fprintf(stderr, "ERROR: %s is returning NULL\n",__FUNCTION__);
			return NULL;
		}
		
		if(doctext){
			ret = (char*)doctext;
			ret += strlen(doctext) ; 
		}
	} else {
		/* ToDo: Test */
		if(doctext){
			ret = realloc(ret, len+strlen(doctext)+1);
			doctext = ret;
			ret += len;
		} else {
			ret = malloc((len+1)*sizeof(char));
		}
	}
//	fprintf(stderr, "%s: ret %p, text %p, len+1: %d\n", __FUNCTION__,ret, text, len+1);
	memcpy(ret, text, len+1);
    ret[len] = '\0';
    
    if(!doctext)
		return ret;
    
    return (char *)doctext;   
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


int xml_doc_print(struct xml_doc  *doc, char* buf, size_t maxsize)
{
    if(!doc)
	return 0;

    if(doc->used < sizeof(struct xml_item))
		return 0;

    if(doc) if(doc->dbglev >= DBGLEV_ITEM )
	fprintf(stderr, "before xml_item_print\n");

    return xml_item_print(doc->first, buf,  maxsize );
}



void xml_doc_printf(struct xml_doc  *doc)
{
	char buf[100000];
	
	xml_doc_print(doc, buf, sizeof(buf));

	printf("%s\n",  buf);
	
}





int xml_doc_hasroot(struct xml_doc  *doc)
{
	if(!doc)
		return 0;

	if(doc->first)
		return 1;
	else 
		return 0;
}


int xml_doc_roothasitems(struct xml_doc  *doc)
{

	if(!xml_doc_hasroot(doc))
		return 0;

	if(doc->first->childs)
		return 1;

	return 0;

}



/* struct xml_stack * xml_doc_parse_prep(size_t stacksize, size_t docsize, int dbglev){ */
    
/*     struct xml_stack *stack = xml_stack_create(stacksize,docsize, dbglev); */

/*     XML_Parser p = XML_ParserCreate("UTF-8"); */
/*     stack->parser = p; */
    
/*     xml_doc_parse_init(p, stack); */
    
/*     return stack; */
/* } */



/* int xml_doc_parse_free(struct xml_stack *stack) */
/* { */
   
/* 	if(!stack) */
/* 		return 0;  */

/* 	if(stack->parser) */
/* 		XML_ParserFree(stack->parser); */

/*     xml_stack_delete(stack); */

/* 	stack = NULL; */

/*     return 0; */
/* } */




