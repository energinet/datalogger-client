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
#include "xml_stack.h"
#include "xml_doc.h"
#include "xml_item.h"

#include <stdio.h>
#include <assert.h>

void xml_stack_parse_init(struct xml_stack *stack);
void xml_stack_parse_deinit(struct xml_stack *stack);



struct xml_stack  *xml_stack_create(size_t stacksize, struct xml_doc  *doc, int dbglev)
{
    struct xml_stack  *stack = malloc(sizeof(*stack));
    assert(stack);

    stack->stack = malloc(sizeof(struct xml_stack*)*stacksize);
    stack->depth = -1;

//    stack->doc = xml_doc_create(docsize, dbglev);
	stack->doc = doc;

	xml_stack_parse_init(stack);

    return stack;

}


void xml_stack_delete(struct xml_stack *stack)
{
    if(!stack)
		return;

	xml_stack_parse_deinit(stack);

    free(stack->stack);
    free(stack);
}


void xml_stack_reset(struct xml_stack  *stack)
{
    stack->depth = -1;
    xml_doc_reset(stack->doc);

	xml_stack_parse_deinit(stack);
	xml_stack_parse_init(stack);
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





void xml_stack_tag_start(void *data, const char *el, const char **attr)
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




void xml_stack_char_hndl(void *data, const char *txt, int txtlen) 
{
    struct xml_stack *stack = (struct xml_stack *)data;
    struct xml_doc *doc = stack->doc;
    struct xml_item *cur  = xml_stack_cur(stack);

    xml_item_add_text(doc, cur, txt, txtlen);


}


void xml_stack_tag_end(void *data, const char *el) 
{
    struct xml_stack *stack = (struct xml_stack *)data;
    xml_stack_pop(stack);
}




void xml_stack_parse_init(struct xml_stack *stack)
{
	XML_Parser p = XML_ParserCreate("UTF-8");
    XML_SetUserData(p, (void *) stack);
    XML_SetElementHandler(p, xml_stack_tag_start, xml_stack_tag_end);
    XML_SetCharacterDataHandler(p, xml_stack_char_hndl); 
	stack->parser = p;
}

void xml_stack_parse_deinit(struct xml_stack *stack)
{
	if(stack->parser)
		XML_ParserFree(stack->parser);
	stack->parser = NULL;
}



int xml_stack_parse_step(struct xml_stack *stack, char *buf, int size){
    
    int retval = 0;


    if((retval = XML_Parse(stack->parser, buf, size, 0))!=XML_STATUS_OK) {
      fprintf(stderr, "Parse error (%d)at line %d:%s\n", retval,
                (int)XML_GetCurrentLineNumber(stack->parser),
                XML_ErrorString(XML_GetErrorCode(stack->parser)));
      fprintf(stderr, "Input '%s' size %d\n",buf, size);
      retval = -5;
    }
    
    

    if(retval<0)
	return retval;
    
    if(stack->depth == -1 && !stack->doc->first)
		return 0;
	

    return stack->depth;
}
