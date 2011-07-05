/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2011 LIAB ApS <info@liab.dk>
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

#ifndef DB_SQLITE_H_
#define DB_SQLITE_H_

#include <sqlite3.h>
#include "module_event.h"

#define DB_TABLE_NAME_EVENT_LOG "event_log"
#define DB_TABLE_COLU_EVENT_LOG "(eid INTEGER, time NUMERIC, value NUMERIC)"

#define DB_TABLE_NAME_EVENT_TYPE "event_type"
#define DB_TABLE_COLU_EVENT_TYPE "(eid INTEGER PRIMARY KEY AUTOINCREMENT, ename TEXT, unit TEXT, descript TEXT)"

#define DB_TABLE_NAME_TIME_INDEX "event_time_index"
#define DB_TABLE_COLU_TIME_INDEX "( time NUMERIC, log_row INTEGER)"

#define DB_CREATE_TABLE_CMD "CREATE TABLE IF NOT EXISTS"

struct query_result {
	int entry_count;
	char *first;
	struct query_resitem *items;
};

struct query_resitem {
	char *col;
	char *value;
	struct query_resitem *next;
};

struct db_sqlite_object {
	sqlite3 *db;
	int verbose;
};

struct query_resitem *query_resitem_create(const char *col, const char *value);
struct query_resitem *query_resitem_add(struct query_resitem *first,
		struct query_resitem *new);
void query_resitem_delete(struct query_resitem *item);

int db_sqlite_cmd(struct db_sqlite_object *db_obj, const char *db_statement,
		struct query_result *result);

int db_sqlite_open(struct db_sqlite_object *db_obj, const char *path,
		int verbose);

int db_sqlite_close(struct db_sqlite_object *db_obj);

int db_sqlite_update_event_type(struct db_sqlite_object *db_obj,
		const char *name, const char *unit, const char *hname);
int db_sqlite_create_event_type(struct db_sqlite_object *db_obj,
		const char *name, const char *unit, const char *hname);
int db_sqlite_get_event_id(struct db_sqlite_object *db_obj, const char *ename);

#endif /* DB_SQLITE_H_ */

