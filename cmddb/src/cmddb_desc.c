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
#include "cmddb_desc.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sqlite3.h>
#include <stdlib.h>



extern int dbglev;
extern struct sqlite3 *cdb;


struct cmddb_desc *cmddb_desc_create(const char *name, int(*handler)(CMD_HND_PARAM), void* userdata, unsigned long flags)
{
    struct cmddb_desc *new;
    if(!name){
	fprintf(stderr, "ERR: creating command descriptor without name\n");
	return NULL;
    }
    new =  malloc(sizeof(*new));
    assert(new);
    
    new->name     = strdup(name);
    new->flags    = flags;
    new->handler  = handler;
    new->userdata = userdata;
    new->next     = NULL;

    return new;
    
}

struct cmddb_desc *cmddb_desc_add(struct cmddb_desc *list, struct cmddb_desc *new)
{
    struct cmddb_desc *ptr = list;

    if(!list)
	return new;
  
    while(ptr->next)
	ptr = ptr->next;

    ptr->next = new;

    return list;
}

void cmddb_desc_delete(struct cmddb_desc *desc)
{
    if(!desc)
	return;

    cmddb_desc_delete(desc->next);
    free(desc->name);
    free(desc);
    
}


struct cmddb_desc *cmddb_desc_get(struct cmddb_desc *list, const char *name)
{
    int index = 0;
    
    while(list){
	if(strcmp(list->name, name)==0)
	    return list;
	list = list->next;
    }

    return NULL;
}

unsigned long  cmddb_desc_get_flags(struct cmddb_desc *list, const char *name)
{
    struct cmddb_desc *desc = cmddb_desc_get(list,name);

    if(desc)
	return desc->flags;

    return 0;
}

/**
 * Get commands to execute
 * @param list : list of enabled commands 
 * @param now  : time (now or a point in time)
 */
int cmddb_exec_list(struct cmddb_desc *list, void *sessdata, time_t now);
