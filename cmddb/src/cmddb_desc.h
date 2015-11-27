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

#ifndef CMDDB_DESC_H_
#define CMDDB_DESC_H_
#include <time.h>
#include "cmddb.h"

#define CMD_HND_PARAM struct cmddb_cmd* cmd, void* userdata, void* sessdata, char** retstr

struct cmddb_desc{
    char *name;
    unsigned long flags;
    int (*handler)(CMD_HND_PARAM);
    void* userdata;
    struct cmddb_desc *next;
};

struct cmddb_cmd *cmddb_cmd_create(int cid, const char *name, const char* value, int status, 
				   time_t stime, time_t ttime, const char *retval);

void cmddb_cmd_delete(struct cmddb_cmd *cmd);

struct cmddb_desc *cmddb_desc_create(const char *name, int(*handler)(CMD_HND_PARAM), void* userdata, unsigned long flags);


struct cmddb_desc *cmddb_desc_add(struct cmddb_desc *list, struct cmddb_desc *new);

void cmddb_desc_delete(struct cmddb_desc *desc);



#endif /* CMDDB_DESC_H_ */
