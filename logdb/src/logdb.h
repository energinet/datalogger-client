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

#ifndef LOGDB_H_
#define LOGDB_H_

#include <time.h>
#include <sqlite3.h>

#define DEFAULT_DB_FILE "/jffs2/bigdb.sql"

#define DEFAULT_TIMEOUT (15000)
#define DEFAULT_EVT_MAX (40000)
#define DB_QUERY_MAX 512
#define DB_VERSION_MAJOR 0
#define DB_VERSION_MINOR 2

struct logdb {
	struct sqlite3 *db;
	int show_progress;
	char *path;
	int debug_level;
};

/* open database file */
struct logdb *logdb_open(const char *filepath, int timeout, int debug_level);

/* close database file */
void logdb_close(struct logdb *db);

/* Add event */
int logdb_evt_add(struct logdb *db, int eid, time_t time, const char* value);

/* Get a list of events */

/* prepare list */
sqlite3_stmt *logdb_evt_list_prep_send(struct logdb *db, int eid);
int logdb_list_send_end(sqlite3_stmt *ppStmt, struct logdb *db, int eid,
		time_t time);
/* prepare list */
sqlite3_stmt *logdb_evt_list_prep(struct logdb *db, int eid, time_t start,
		time_t end);
/* Get next list */
int logdb_evt_list_next(sqlite3_stmt *ppStmt, int *eid, time_t *time,
		const char **value);

/* end list */
int logdb_list_end(sqlite3_stmt *ppStmt);

/* count number of events */
int logdb_evt_count(struct logdb *db, int eid);

int logdb_evt_interval(struct logdb *db, int eid, time_t *lastlog,
		time_t *lastsend);

int logdb_evt_mark_send(struct logdb *db, int eid, const char *lastsend);

/* Add event type */
int logdb_etype_add(struct logdb *db, char *name, char *hname, char *unit,
		char *type);

/* Get eid */
int logdb_etype_get_eid(struct logdb *db, const char *ename);

/* Remove event type and logged data */
int logdb_etype_rem(struct logdb *db, int eid);

/* Get a list of event types */
/* prepare list */
sqlite3_stmt *logdb_etype_list_prep(struct logdb *db);

/* Get next list */
int logdb_etype_list_next(sqlite3_stmt *ppStmt, int *eid, char **ename,
		char **hname, char **unit, char **type);

/* end list */
int logdb_etype_list_end(struct logdb *db);

int logdb_etype_count(struct logdb *db);

int logdb_setting_set(struct logdb *db, const char *name, const char *value);
char *logdb_setting_get(struct logdb *db, const char *name, char* value,
		int len);

int
		logdb_get_first_int(struct logdb *db, const char *sql, int len,
				int *result);

int logdb_check_integrity(struct logdb *db);

int logdb_evt_remove_old(struct logdb *db, time_t oletime);

int logdb_evt_maintain(struct logdb *db);

#endif /* LOGDB_H_ */
