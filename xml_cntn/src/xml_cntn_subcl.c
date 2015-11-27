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
#include "xml_cntn_rcvr.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct xml_cntn_subcl *xml_cntn_subcl_create(enum xml_cntn_subcl_type type, const char *tag)
{
	struct xml_cntn_subcl *new = malloc(sizeof(*new));

	new->type = type;
	new->rcvcnt = 0;
	if(tag)
		new->seqtag = strdup(tag);
	else 
		new->seqtag = NULL;
	new->item = NULL;
	new->next = NULL;
	
	return new;
}


struct xml_cntn_subcl *xml_cntn_subcl_create_seq()
{
	return xml_cntn_subcl_create(SCLTYPE_SEQ, NULL);	
}

struct xml_cntn_subcl *xml_cntn_subcl_create_subs(const char *tag)
{
	return xml_cntn_subcl_create(SCLTYPE_SUBS, tag);	
}

void xml_cntn_subcl_delete(struct xml_cntn_subcl *subcl)
{
	if(!subcl)
		return;
	
	assert(!subcl->active);
	
	xml_cntn_subcl_delete(subcl->next);

	free(subcl->seqtag);
	free(subcl);

}


struct xml_cntn_subcl *xml_cntn_subcl_list_add(struct xml_cntn_subcl *list, struct xml_cntn_subcl *new)
{
	struct xml_cntn_subcl *ptr = list;

    if(!ptr){
		return new;
    }
	
    while(ptr->next)
		ptr = ptr->next;
	
    ptr->next = new;
	
    return list;
}

struct xml_cntn_subcl *xml_cntn_subcl_list_rem(struct xml_cntn_subcl *list, struct xml_cntn_subcl *rem)
{
    struct xml_cntn_subcl *ptr = list;

    if(!rem){
		return list;
    }
	
    if(ptr == rem){
		struct xml_cntn_subcl *next = ptr->next;
		ptr->next = NULL;
		return next;
    }

    while(ptr->next){
		if(ptr->next == rem){
			struct xml_cntn_subcl *found = ptr->next;
			ptr->next = found->next;
			found->next = NULL;
			break;
		}
		ptr = ptr->next;
    }
    return list;
}


void xml_cntn_subcl_txprep(struct xml_doc* doc ,struct xml_item *item, struct xml_cntn_subcl *subcl)
{
	if(!subcl)
		return;

	if(subcl->type == SCLTYPE_SEQ){
		xml_item_add_attr(doc, item, "seq", subcl->seqtag);
	}
}


int xml_cntn_subcl_match(struct xml_cntn_subcl *subcl, const char *seq, const char *tag)
{
	switch(subcl->type){
	  case SCLTYPE_SEQ:
		if(!seq)
			return 0;
		if(strcmp(seq, subcl->seqtag)==0)
			return 1;
		return 0;		
	  case SCLTYPE_SUBS:
		if(strcmp(tag, subcl->seqtag)==0)
			return 1;
		return 0;
	  default:
		fprintf(stderr, "Error in %s: Unknown type %d\n", __FUNCTION__, subcl->type);
		return 0;
	}
}


void xml_cntn_subcl_activate(struct xml_cntn_subcl* subcl, unsigned long  seq)
{
	subcl->active = 1;
	
	if(subcl->type == SCLTYPE_SEQ){
		char seq_str[32];		
		sprintf(seq_str, "%lu", seq);
		subcl->seqtag = strdup(seq_str);
	}
	
}

void xml_cntn_subcl_cancel(struct xml_cntn_subcl* subcl)
{
	subcl->active = 0;
}
