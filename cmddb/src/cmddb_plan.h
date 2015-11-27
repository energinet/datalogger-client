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

#ifndef CMDDB_PLAN_H_
#define CMDDB_PLAN_H_
#include <time.h>
#include "cmddb.h"


struct cmddb_plan{
    int rowid;
	char *value;
    time_t ttime;    
	struct cmddb_plan *next;
};



/***********************************
 * Command database plan functions */

struct cmddb_plan *cmddb_plan_create(int rowid, time_t ttime, const char *value);

struct cmddb_plan *cmddb_plan_add(struct cmddb_plan *list ,struct cmddb_plan *new);
void cmddb_plan_delete(struct cmddb_plan *plan);

/**
 * Write a plan to the database
 */
int cmddb_plan_set(struct cmddb_plan *plan, const char *cmd_name, time_t t_begin, time_t t_end);


struct cmddb_plan *cmddb_plan_get(const char *cmd_name, time_t t_begin, time_t t_end);




#endif /* CMDDB_H_ */
