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
#include "sequence_io.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include <module_base.h>
#include <module_value.h>
struct seq_io *seq_io_create_input( struct module_base *base, const char *event)
{
    struct seq_io *new = malloc(sizeof(*new));
    assert(new);

    new->evalue = evalue_create(base, event,  NULL, event,  DEFAULT_FEVAL);
    if(!new->evalue){
	PRINT_ERR("Could not create evalue");
	goto err_out;
    }

    new->name = strdup(event);

 err_out:
    free(new);
    return NULL;
    

}



struct seq_io *seq_io_create_attr( struct module_base *base, const char **attr)
{
	struct seq_io *new = malloc(sizeof(*new));
	assert(new);

	new->evalue = evalue_create_attr(base, get_attr_str_ptr(attr, "event"), attr);
	new->name = NULL;

	return new;
}

struct seq_io *seq_io_lst_add(struct seq_io *list, struct seq_io *new)
{
    if(!list){
	return new;
    }
    
    new->next = list;
    
    return new;

}

void seq_io_delete(struct seq_io *io)
{
    if(!io){
	return ;
    }
    seq_io_delete(io->next);
    evalue_delete(io->evalue);
    free(io->name);
    free(io);
}

const char *seq_io_get_name(struct seq_io *io)
{
    if(!io)
	return NULL;

    if(io->name)
	return io->name;
    
    if(io->evalue)
	return io->evalue->name;
    return NULL;
}

struct seq_io *seq_io_get_by_name(struct seq_io *list, const char *name)
{
    struct seq_io *ptr = list;
    
    if(!name)
	return NULL;
  	
    while(ptr){
	const char *ename = seq_io_get_name(ptr);
	if(name)
	    if(strcmp(ename,  name)==0)
		return ptr;
	ptr = ptr->next;
    }
	
    return NULL;
}


void seq_io_setf(struct seq_io *io, float value)
{
    evalue_setf(io->evalue, value);
}


float seq_io_getf(struct seq_io *io)
{
    return evalue_getf(io->evalue);
}


int seq_io_wait(struct seq_io *io, float value)
{
    if(isnan(value))
	return evalue_wait(io->evalue, 0);
    else
	return evalue_wait_value(io->evalue, value , 0);
}


