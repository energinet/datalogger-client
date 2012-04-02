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

#include "timevar.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>


void timevar_init(struct timevar *var, const char *value)
{
    assert(var);
	strncpy(var->value, value, sizeof(var->value));

    return;
}

void timevar_set(struct timevar *var, const char *value, time_t t_update)
{
    assert(var);
	strncpy(var->value, value, sizeof(var->value));
    var->t_update = t_update;
}



void timevar_setf(struct timevar *var, float value, time_t t_update)
{
	char buf[128];
	
	snprintf(buf, sizeof(buf), "%f", value);
	timevar_set(var, buf, t_update);
}


void timevar_read_mark(struct timevar *var)
{
	assert(var);
	var->t_read =  var->t_update;
}

void timevar_read(struct timevar *var, char *buf, int buf_size)
{
	timevar_read_mark(var);
	assert(buf);
	strncpy(buf, var->value, buf_size);
}

float timevar_readf(struct timevar *var)
{
	float value;
	timevar_read_mark(var);    
	sscanf(var->value, "%f", &value);
    return value;
}


char *timevar_readcp(struct timevar *var)
{	
	timevar_read_mark(var);    
	return strdup(var->value);
}

int timevar_updated(struct timevar *var)
{
    return var->t_update != var->t_read;
}
