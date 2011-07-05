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

#include "db-sqlite.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h>
struct query_resitem *query_resitem_create(const char *col, const char *value) {

	struct query_resitem *new = malloc(sizeof(*new));

	if (!col) {
		PRINT_ERR("col == NULL");
		return NULL;
	}

	if (!new) {
		PRINT_ERR("malloc failed");
		return NULL;
	}

	memset(new, 0, sizeof(*new));

	new->col = strdup(col);
	if (value)
		new->value = strdup(value);

	return new;

}

struct query_resitem *query_resitem_add(struct query_resitem *first,
		struct query_resitem *new) {

	struct query_resitem *ptr = first;

	if (!first)
		/* first module in the list */
		return new;

	/* find the last module in the list */
	while (ptr->next) {
		ptr = ptr->next;
	}

	ptr->next = new;

	return first;
}

void query_resitem_delete(struct query_resitem *item) {

	if (item) {
		query_resitem_delete(item->next);
		free(item->col);
		free(item->value);
		free(item);
	}

}

int db_callback(void *data, int argc, char **argv, char **azColName) {
	struct query_result *result = (struct query_result*) data;
	int i;

	if (result)
		result->entry_count++;

	for (i = 0; i < argc; i++) {
		PRINT_DBG(0,"%s = %s", azColName[i], argv[i] ? argv[i] : "NULL");

		if (result) {
			result->items = query_resitem_add(result->items,
					query_resitem_create(azColName[i], argv[i]));
			if (result->first == NULL && argv[i])
				result->first = strdup(argv[i]);
		}
	}

	return 0;

}

int db_sqlite_cmd(struct db_sqlite_object *db_obj, const char *db_statement,
		struct query_result *result) {
	int retval = -EFAULT;
	char *zErrMsg = NULL;

	if (result)
		memset(result, 0, sizeof(*result));

	if (db_obj->verbose)
		printf("sql query: %s\n", db_statement);

	retval = sqlite3_exec(db_obj->db, db_statement, db_callback, result,
			&zErrMsg);

	if (retval != SQLITE_OK) {
		PRINT_ERR("SQL error: %s", zErrMsg);
		PRINT_ERR("SQL query: %s", db_statement);
		sqlite3_free(zErrMsg);
		retval = -EFAULT;
	} else {
		retval = 0;
	}

	return 0;

}

int db_sqlite_init_tables(struct db_sqlite_object *db_obj) {
	int retval = -EFAULT;

	retval
			= db_sqlite_cmd(
					db_obj,
					DB_CREATE_TABLE_CMD " " DB_TABLE_NAME_EVENT_LOG " " DB_TABLE_COLU_EVENT_LOG,
					NULL);
	if (retval < 0) {
		PRINT_ERR("error creating table %s", DB_TABLE_NAME_EVENT_LOG);
		return retval;
	}

	retval
			= db_sqlite_cmd(
					db_obj,
					DB_CREATE_TABLE_CMD " " DB_TABLE_NAME_EVENT_TYPE " " DB_TABLE_COLU_EVENT_TYPE,
					NULL);
	if (retval < 0) {
		PRINT_ERR("error creating table %s", DB_TABLE_NAME_EVENT_TYPE);
		return retval;
	}

	retval
			= db_sqlite_cmd(
					db_obj,
					DB_CREATE_TABLE_CMD " " DB_TABLE_NAME_TIME_INDEX " " DB_TABLE_COLU_TIME_INDEX,
					NULL);
	if (retval < 0) {
		PRINT_ERR("error creating table %s", DB_TABLE_NAME_TIME_INDEX);
		return retval;
	}

	return 0;
}

int db_sqlite_open(struct db_sqlite_object *db_obj, const char *path,
		int verbose) {
	int retval = -EFAULT;
	db_obj->verbose = verbose;

	PRINT_DBG(db_obj->verbose, "is verbose");

	retval = sqlite3_open(path, &db_obj->db);

	if (retval) {
		PRINT_ERR("sqlite3_open returned %d", retval);
		sqlite3_close(db_obj->db);
		return retval;
	}

	sqlite3_busy_timeout(db_obj->db, 15000);

	db_sqlite_init_tables(db_obj);

	return 0;

}

struct query_resitem *db_sqlite_get_list(struct db_sqlite_object *db_obj,
		const char *query) {
	struct query_result qresult;
	int retval;

	if ((retval = db_sqlite_cmd(db_obj, query, &qresult)) < 0)
		return NULL;

	if (qresult.first) {
		free(qresult.first);
	}

	return qresult.items;

}

int db_sqlite_get_first_int(struct db_sqlite_object *db_obj, const char *query,
		int *result) {
	struct query_result qresult;
	int retval = 0;

	if ((retval = db_sqlite_cmd(db_obj, query, &qresult)) < 0)
		return retval;

	if (qresult.first) {
		*result = atoi(qresult.first);
		free(qresult.first);
	}

	query_resitem_delete(qresult.items);

	return qresult.entry_count;

}

int db_sqlite_close(struct db_sqlite_object *db_obj) {
	sqlite3_close(db_obj->db);
}

int db_sqlite_update_event_type(struct db_sqlite_object *db_obj,
		const char *name, const char *unit, const char *hname) {
	char query[512];

	int eid = db_sqlite_get_event_id(db_obj, name);

	if (eid < 0)
		snprintf(
				query,
				512,
				"INSERT INTO %s (ename, unit, descript) VALUES ('%s', '%s' , '%s')",
				DB_TABLE_NAME_EVENT_TYPE, name, unit, hname);
	else
		snprintf(query, 512, "UPDATE %s SET descript='%s' WHERE rowid=%d",
				DB_TABLE_NAME_EVENT_TYPE, hname, eid);

	return db_sqlite_cmd(db_obj, query, NULL );

}

int db_sqlite_create_event_type(struct db_sqlite_object *db_obj,
		const char *name, const char *unit, const char *hname) {
	char query[512];
	snprintf(
			query,
			512,
			"INSERT INTO %s (ename, unit, descript) VALUES ('%s', '%s' , '%s')",
			DB_TABLE_NAME_EVENT_TYPE, name, unit, hname);

	return db_sqlite_cmd(db_obj, query, NULL );

}

int db_sqlite_get_event_id(struct db_sqlite_object *db_obj, const char *ename) {
	char query[512];
	int eid = -1;
	int retval = 0;
	snprintf(query, 512, "SELECT eid FROM %s WHERE ename ='%s'",
			DB_TABLE_NAME_EVENT_TYPE, ename);

	retval = db_sqlite_get_first_int(db_obj, query, &eid);

	if (retval == 0) {
		if (db_obj->verbose)
			printf("ename == %s does not exist", ename);
		return -1;
	} else if (retval < 0) {
		PRINT_ERR("db error getting eid for %s", ename);
		return -2;
	}

	if (retval > 1)
		PRINT_ERR("%s har multible entrys !!!", ename);

	if (db_obj->verbose)
		printf("%s has eid %d", ename, eid);

	return eid;

}
