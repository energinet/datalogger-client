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
 
#include "logdb.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <assert.h>

#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s: "str"\n",__FUNCTION__, ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}

struct query_resitem {
	char *col;
	char *value;
	struct query_resitem *next;
};

struct query_result {
	int entry_count;
	struct query_resitem *items;
};
#define DB_CREATE_TABLE_CMD "CREATE TABLE IF NOT EXISTS"

#define DB_TABLE_NAME_EVENT_LOG "event_log"
#define DB_TABLE_COLU_EVENT_LOG "(eid INTEGER, time INTEGER, value NUMERIC)"

#define DB_TABLE_NAME_EVENT_TYPE "event_type"
#define DB_TABLE_COLU_EVENT_TYPE "(eid INTEGER PRIMARY KEY AUTOINCREMENT, ename TEXT, unit TEXT, type TEXT, hname TEXT, interval INTEGER, lastlog INTEGER, lastsend INTEGER)"

#define DB_TABLE_NAME_DB_SETTING "settings"
#define DB_TABLE_COLU_DB_SETTING "(ename TEXT PRIMARY KEY, value TEXT)"

int logdb_progress_handler(void* user) {
	struct logdb *db = (struct logdb*) user;

	if (db->show_progress) {
		fprintf(stderr, ".");
	}

	return 0;
}

struct query_resitem *logdb_resitem_create(const char *col, const char *value) {

	struct query_resitem *new = malloc(sizeof(*new));

	if (!col) {
		printf("col == NULL");
		return NULL;
	}

	if (!new) {
		printf("malloc failed");
		return NULL;
	}

	printf("resitem_create\n");

	memset(new, 0, sizeof(*new));

	new->col = strdup(col);
	if (value)
		new->value = strdup(value);

	return new;

}

struct query_resitem *logdb_resitem_add(struct query_resitem *first,
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

int logdb_sql_callback(void *data, int argc, char **argv, char **azColName) {
	struct query_result *result = (struct query_result*) data;
	int i;

	if (result)
		result->entry_count++;

	for (i = 0; i < argc; i++) {
		if (result) {
			result->items = logdb_resitem_add(result->items,
					logdb_resitem_create(azColName[i], argv[i]));
		}
	}

	return 0;

}

int logdb_sql(struct logdb *db, const char *sql, struct query_result *result) {
	int retval;
	char *zErrMsg = NULL;

	if (result)
		memset(result, 0, sizeof(*result));

	PRINT_DBG(db->debug_level, "Exec sql: %s", sql);

	retval = sqlite3_exec(db->db, sql, logdb_sql_callback, result, &zErrMsg);

	if (retval != SQLITE_OK) {
		PRINT_ERR("SQL error: %s", zErrMsg);
		PRINT_ERR("SQL query: %s", sql);
		sqlite3_free(zErrMsg);
		retval = -EFAULT;
	} else {
		retval = 0;
	}

	return retval;

}

int logdb_get_first_int(struct logdb *db, const char *sql, int len, int *result) {
	sqlite3_stmt *stathandel;
	const char *pzTail;
	int retval = -1;
	if (!len)
		len = strlen(sql);

	PRINT_DBG(db->debug_level, "Exec sql: %s", sql);
	retval = sqlite3_prepare_v2(db->db, sql, len, &stathandel, &pzTail);

	if (retval != SQLITE_OK) {
		return -EFAULT;
	}

	retval = sqlite3_step(stathandel);

	switch (retval) {
	case SQLITE_ROW:
		*result = sqlite3_column_int(stathandel, 0);
		retval = 0;
		break;
	case SQLITE_DONE:
		retval = -EFAULT;
		break;
	case SQLITE_BUSY:
	case SQLITE_ERROR:
	case SQLITE_MISUSE:
	default:
		retval = -EFAULT;
		break;
	}

	PRINT_DBG(db->debug_level, "retval %d (%d) retval %d", retval, SQLITE_ROW, *result );

	sqlite3_finalize(stathandel);

	return retval;

}

/*
 * return -4 if error
 *        -3 if no version
 *        -2 if major version is older
 *        -1 if minor version is older
 *         0 if version is the same
 *         1 if minor version is newer
 *         2 if major version is newer
 */

int logdb_check_version(struct logdb *db) {
	char *db_version = logdb_setting_get(db, "version", NULL, 0);
	int db_ver_major = 0;
	int db_ver_minor = 0;

	if (db_version == NULL) {
		PRINT_DBG(db->debug_level,"No DB version");
		return -4;
	}

	if (sscanf(db_version, "%d.%d", &db_ver_major, &db_ver_minor) != 2)
		return -3;

	free(db_version);

	if (db_ver_major == DB_VERSION_MAJOR) {
		if (db_ver_minor == DB_VERSION_MINOR)
			return 0;
		else if (db_ver_minor > DB_VERSION_MINOR)
			return 1;
		else
			return -1;
	} else if (db_ver_major > DB_VERSION_MAJOR)
		return 2;
	else
		return -2;

}

int logdb_init(struct logdb *db) {
	int retval;
	char db_version[32];

	snprintf(db_version, 32, "%d.%d", DB_VERSION_MAJOR, DB_VERSION_MINOR);

	PRINT_DBG(db->debug_level, "Init db (version %s)",db_version );

	retval = logdb_sql(db, DB_CREATE_TABLE_CMD " " DB_TABLE_NAME_EVENT_LOG
	" " DB_TABLE_COLU_EVENT_LOG, NULL);
	if (retval < 0) {
		PRINT_ERR("error creating table %s", DB_TABLE_NAME_EVENT_LOG);
		return retval;
	}

	retval = logdb_sql(db, DB_CREATE_TABLE_CMD " " DB_TABLE_NAME_EVENT_TYPE
	" " DB_TABLE_COLU_EVENT_TYPE, NULL);
	if (retval < 0) {
		PRINT_ERR("error creating table %s", DB_TABLE_NAME_EVENT_TYPE);
		return retval;
	}

	retval = logdb_sql(db, DB_CREATE_TABLE_CMD " " DB_TABLE_NAME_DB_SETTING
	" " DB_TABLE_COLU_DB_SETTING, NULL);
	if (retval < 0) {
		PRINT_ERR("error creating table %s", DB_TABLE_NAME_DB_SETTING);
		return retval;
	}

	retval = logdb_sql(db, "CREATE INDEX IF NOT EXISTS evti"
					   " ON event_log (eid)" , NULL);
	if (retval < 0) {
		PRINT_ERR("error creating index %s", "evti");
		return retval;
	}

	
	retval = logdb_sql(db, "PRAGMA default_cache_size=10000", NULL);

	if (retval < 0) {
		PRINT_ERR("error setting default_cache_size=10000");
		return retval;
	}

	logdb_setting_set(db, "version", db_version);

	return 0;
}

/* open database file */
struct logdb *logdb_open(const char *filepath, int timeout, int debug_level) {
	int retval;
	struct logdb *db = malloc(sizeof(*db));

	assert(db);
	memset(db, 0, sizeof(*db));

	db->debug_level = debug_level;

    if(!filepath)
		filepath = DEFAULT_DB_FILE;

	PRINT_DBG(debug_level, "Opening database file %s",filepath);

	retval = sqlite3_open(filepath, &db->db);

	if (retval) {
		PRINT_ERR("sqlite3_open returned %d", retval);
		goto err_out;
	}

	if (timeout == -1)
		timeout = DEFAULT_TIMEOUT;

	if (timeout != 0) {
		PRINT_DBG(debug_level, "Setting timeout to %d",timeout);
		sqlite3_busy_timeout(db->db, timeout);
	}

	retval = logdb_check_version(db);

	if (retval != 0) {
		retval = logdb_init(db);

		if ((retval = logdb_init(db)) < 0) {
			goto err_out;
		}
	}
	db->show_progress = 0;
	sqlite3_progress_handler(db->db, 5000, logdb_progress_handler, db);

	PRINT_DBG(debug_level, "DB has adr %p",db);

	return db;

	err_out: sqlite3_close(db->db);
	free(db);

	return NULL;

}

/* close database file */
void logdb_close(struct logdb *db) {
	assert(db);

	PRINT_DBG(db->debug_level, "Closing DB");
	sqlite3_close(db->db);

	db->db = NULL;

	free(db->path);

	free(db);

}

/* Add event */
int logdb_evt_add(struct logdb *db, int eid, time_t time, const char* value) {
	int retval;
	char sql[DB_QUERY_MAX];

	snprintf(sql, DB_QUERY_MAX, "UPDATE %s SET lastlog=%ld WHERE rowid=%d",
			DB_TABLE_NAME_EVENT_TYPE, time, eid);

	retval = logdb_sql(db, sql, NULL);
	if (retval < 0) {
		PRINT_ERR("ERROR updating table %s", DB_TABLE_NAME_EVENT_TYPE);
		return retval;
	}


	snprintf(sql, DB_QUERY_MAX, "INSERT INTO %s (eid, time, value) "
		"VALUES ('%d', '%ld' , '%s')", DB_TABLE_NAME_EVENT_LOG, eid, time,
			value);

	return logdb_sql(db, sql, NULL );

}


int logdb_evt_get_last(struct logdb *db, int eid, time_t *time, char *valuebuf, size_t bufmax)
{
    char sql[DB_QUERY_MAX]; 
    int ptr = 0;
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char retval = 0;
    int sqlret;
    ptr += snprintf(sql,DB_QUERY_MAX, "SELECT value, time FROM %s WHERE eid=%d AND "
		    "time = (SELECT lastlog FROM %s WHERE eid = %d)", 
		    DB_TABLE_NAME_EVENT_LOG, eid, DB_TABLE_NAME_EVENT_TYPE, eid);   
    
    PRINT_DBG(db->debug_level, "Get setting, sql: %s", sql);

    sqlite3_prepare_v2(db->db, sql,  ptr , &stathandel, &pzTail );
    
    sqlret =  sqlite3_step(stathandel);

    switch(sqlret){
      case SQLITE_ROW:
	strncpy(valuebuf, (const char*)sqlite3_column_text(stathandel, 0), bufmax);
	*time = sqlite3_column_int(stathandel, 1);
	retval = 0;
        break;
      case SQLITE_DONE:
        PRINT_DBG(db->debug_level, "Get setting not found");
	retval = -1;
        break;
      case SQLITE_BUSY:
      case SQLITE_ERROR:
      case SQLITE_MISUSE:
      default:
        break;
    }

    sqlite3_finalize(stathandel);
    
    PRINT_DBG(db->debug_level, "Setting returned %s", retval);
    return retval;   
}



int logdb_evt_get_flast(struct logdb *db, int eid, time_t *time, float *value)
{
    char buffer[256];
    int retval;
       
    if((retval = logdb_evt_get_last(db, eid, time, buffer, sizeof(buffer)))<0)
	return retval;

    if(sscanf(buffer, "%f", value)!=1)
	return -1;

    return 0;    

}


/* Get a list of events */
sqlite3_stmt *logdb_evt_list_prep_send(struct logdb *db, int eid) {
	char sql[DB_QUERY_MAX];
	char eids[64];
	int sqlp = 0;
	int lastlog = 0;
	int lastsend = 0;
	sqlp = sprintf(sql, "SELECT lastlog FROM event_type WHERE eid=%d", eid);
	logdb_get_first_int(db, sql, sqlp, &lastlog);
	sqlp = sprintf(sql, "SELECT lastsend FROM event_type WHERE eid=%d", eid);
	logdb_get_first_int(db, sql, sqlp, &lastsend);

	sprintf(eids, "%d", eid);

	return logdb_evt_list_prep(db, eids, lastsend, lastlog);

}

/* prepare list */
sqlite3_stmt *logdb_evt_list_prep(struct logdb *db, const char *eids, time_t start,
		time_t end) {
	char sql[DB_QUERY_MAX];
	sqlite3_stmt *ppStmt;
	const char *pzTail;
	int retval;
	int ptr = 0;

	ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr,
			"SELECT eid,time,value FROM %s ", DB_TABLE_NAME_EVENT_LOG);

	if ((eids) || start || end) {
		ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "WHERE ");

		if (eids) {
			ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "eid IN (%s) ", eids);

			if (start || end)
				ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "AND ");
		}

		if (start) {
			ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, " time > '%ld' ",
					start);
			if (end)
				ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "AND ");
		}

		if (end)
			ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, " time <= '%ld' ",
					end);

	}

	retval = sqlite3_prepare_v2(db->db, sql, ptr, &ppStmt, &pzTail);

	if (retval != SQLITE_OK) {
		PRINT_ERR("sqlite3_prepare(%s) returned %d",sql, retval);
		return NULL;
	}

	return ppStmt;
}

int logdb_evt_count(struct logdb *db, int eid) {
	char sql[DB_QUERY_MAX];
	int ptr = 0;
	int result = 0;

	ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "SELECT Count(*) FROM %s ",
			DB_TABLE_NAME_EVENT_LOG);

	if (eid >= 0)
		ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "WHERE eid=%d", eid);

	if (logdb_get_first_int(db, sql, ptr, &result) != 0) {
		return -EFAULT;
	}

	return result;

}

int logdb_evt_interval(struct logdb *db, int eid, time_t *lastlog,
		time_t *lastsend) {
	char sql[DB_QUERY_MAX];
	int sqlp = 0;
	sqlp = sprintf(sql, "SELECT lastlog FROM event_type WHERE eid=%d", eid);
	logdb_get_first_int(db, sql, sqlp, (int*) lastlog);
	sqlp = sprintf(sql, "SELECT lastsend FROM event_type WHERE eid=%d", eid);
	logdb_get_first_int(db, sql, sqlp, (int*) lastsend);

	return 0;
}

/* Get next list */
int logdb_evt_list_next(sqlite3_stmt *ppStmt, int *eid, time_t *time,
		const char **value) {
	int retval;

	retval = sqlite3_step(ppStmt);

	switch (retval) {
	case SQLITE_ROW:
		*eid = sqlite3_column_int(ppStmt, 0);
		*time = sqlite3_column_int(ppStmt, 1);
		*value = (char *) sqlite3_column_text(ppStmt, 2);
		retval = 1;
		break;
	case SQLITE_DONE:
		retval = 0;
		break;
	case SQLITE_BUSY:
	case SQLITE_ERROR:
	case SQLITE_MISUSE:
	default:
		retval = -1;
		break;
	}

	return retval;
}

/* end list */
int logdb_list_end(sqlite3_stmt *ppStmt) {
	sqlite3_finalize(ppStmt);
	return 0;
}

int logdb_list_send_end(sqlite3_stmt *ppStmt, struct logdb *db, int eid,
		time_t time) {
	char sql[DB_QUERY_MAX];

	logdb_list_end(ppStmt);

	if (time) {
		snprintf(sql, DB_QUERY_MAX,
				"UPDATE %s SET lastsend=%ld WHERE rowid=%d",
				DB_TABLE_NAME_EVENT_TYPE, time, eid);

		logdb_sql(db, sql, NULL );
	}
	return 0;

}

int logdb_evt_mark_send(struct logdb *db, int eid, const char *lastsend) {
	char sql[DB_QUERY_MAX];

	snprintf(sql, DB_QUERY_MAX, "UPDATE %s SET lastsend=%s WHERE rowid=%d",
			DB_TABLE_NAME_EVENT_TYPE, lastsend, eid);

	return logdb_sql(db, sql, NULL );

}

/* Get eid */
int logdb_etype_get_eid(struct logdb *db, const char *ename) {
	char sql[DB_QUERY_MAX];
	int ptr = 0;
	int result = -1;
	PRINT_DBG(db->debug_level, "Get eid for %s %d %d %p", ename, ptr, result, sql);
	ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr,
			"SELECT eid FROM %s WHERE ename='%s'", DB_TABLE_NAME_EVENT_TYPE,
			ename);

	if (logdb_get_first_int(db, sql, ptr, &result) != 0) {
		return -EFAULT;
	}

	PRINT_DBG(db->debug_level, "eid is %d", result);

	return result;

}

int logdb_etype_inprep(const char *enames, char *qnames_ret, int qnames_size)
{
	const char *ptr = enames;
	const char *next = enames;
	int retlen = 0;
	int count = 0;
	char *qnames_end = qnames_ret+qnames_size;
	char *qnames_ptr = qnames_ret;
	
	if(!enames)
		return 0;
	fprintf(stderr, "inprep:%s\n", enames);
	
	do{
		int len, i;
		next = strchr(ptr+1,',');
		fprintf(stderr, "next:%s\n", next);
		if(next){
			len = next-ptr;
			next++;
		}else
			len = strlen(ptr);
		
		if(len > ((qnames_end-qnames_ptr)+4))
			break;
		if(len == 0)
			break;

		if(qnames_ptr == qnames_ret){
			qnames_ptr += sprintf(qnames_ptr, "'"); 
		} else {
			qnames_ptr += sprintf(qnames_ptr, ",'"); 
		}

		memcpy(qnames_ptr, ptr, len);

		for(i = 0; i < len; i++){
			if((ptr[i] == '\'')||(ptr[i] == ' ')){
				continue;
			} else {
				qnames_ptr[0] = ptr[i];
				qnames_ptr++;
			}
				
		}

		qnames_ptr += sprintf(qnames_ptr, "'"); 
		count++;
		ptr = next;
	}while(ptr); 
	
	return qnames_ptr -qnames_ret;

}

char *logdb_etype_get_eids(struct logdb *db, const char *enames, char *eids_ret, int eids_size)
{
	char query[1024];
	int len = 0;
	int retlen = 0;
	int count;
	int retval;
	sqlite3_stmt *ppStmt;
	const char *pzTail;
	
	len += sprintf(query+len, "SELECT eid FROM event_type WHERE ename IN (");
	
	count = logdb_etype_inprep(enames, query+len, sizeof(query)-len);

	if(!count)
		return NULL;
	len += count;

	len += sprintf(query+len, ");");
	
	fprintf(stderr, "Query: %s (count: %d)\n", query, count);

	retval = sqlite3_prepare_v2(db->db, query, len, &ppStmt, &pzTail);

	count= 0;
	while(sqlite3_step(ppStmt) == SQLITE_ROW){
		int eid = sqlite3_column_int(ppStmt, 0);
		retlen += snprintf(eids_ret+retlen, eids_size-retlen, (retlen)?",%d":"%d" , eid);
		count++;
	}

	
	fprintf(stderr, "Result: %s  (count: %d)\n", eids_ret, count);
	if(count)
		return eids_ret;
	else
		return NULL;
}

/* Add event type */
int logdb_etype_add(struct logdb *db, char *ename, char *hname, char *unit,
		char *type) {
	char sql[DB_QUERY_MAX];
	char time_str[32];
	struct timeval tv;
	int eid = logdb_etype_get_eid(db, ename);
	int ptr = 0;

	/* update timestamp */
	sprintf(time_str, "%ld", time(NULL));
	logdb_setting_set(db, "etype_updated", time_str);

	if (eid < 0) {
		const char *_hname;
		const char *_unit;
		const char *_type;
		time_t init_time = 0;

		if (hname)
			_hname = hname;
		else
			_hname = ename;

		if (unit)
			_unit = unit;
		else
			_unit = "";

		if (type)
			_type = type;
		else
			_type = "";

		PRINT_DBG(db->debug_level, "Adding etype: %s", ename);

		ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr,
				"INSERT INTO %s (ename, unit, hname, type, lastlog, lastsend) "
					"VALUES ('%s', '%s' , '%s', '%s', %lu , %lu)",
				DB_TABLE_NAME_EVENT_TYPE, ename, _unit, _hname, _type,
				init_time, init_time);
	} else {
		PRINT_DBG(db->debug_level, "Updating etype: %s (%d)", ename, eid);
		/* update */
		ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "UPDATE %s SET ",
				DB_TABLE_NAME_EVENT_TYPE);

		if (hname) {
			ptr
					+= snprintf(sql + ptr, DB_QUERY_MAX - ptr, "hname='%s' ",
							hname);
			if (unit || type)
				ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, ",");
		}

		if (unit) {
			ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "unit='%s' ", unit);
			if (type)
				ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, ",");
		}

		if (type) {
			ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "type='%s' ", type);
		}

		ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "WHERE rowid=%d", eid);
	}

	return logdb_sql(db, sql, NULL);

}

/* Remove event type and logged data */
int logdb_etype_rem(struct logdb *db, int eid) {
	char where[64];

	sprintf(where, "eid=%d ", eid);

	printf("deleting from etypes \n");
	logdb_delete(db, DB_TABLE_NAME_EVENT_TYPE, where);

	printf("deleting from elogs \n");
	logdb_delete(db, DB_TABLE_NAME_EVENT_LOG, where);

	return 0;
}

/* Get a list of event types */
/* prepare list */
sqlite3_stmt *logdb_etype_list_prep(struct logdb *db) {
	char sql[DB_QUERY_MAX];
	sqlite3_stmt *ppStmt;
	const char *pzTail;
	int retval;
	int ptr = 0;

	ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr,
			"SELECT eid,ename,hname,unit,type FROM %s ",
			DB_TABLE_NAME_EVENT_TYPE);

	retval = sqlite3_prepare_v2(db->db, sql, ptr, &ppStmt, &pzTail);

	if (retval != SQLITE_OK) {
		PRINT_ERR("sqlite3_prepare(%s) returned %d",sql, retval);
		return NULL;
	}

	return ppStmt;
}

/* Get next list */
int logdb_etype_list_next(sqlite3_stmt *ppStmt, int *eid, char **ename,
		char **hname, char **unit, char **type) {
	int retval;

	retval = sqlite3_step(ppStmt);

	switch (retval) {
	case SQLITE_ROW:
		*eid = sqlite3_column_int(ppStmt, 0);
		*ename = (char*) sqlite3_column_text(ppStmt, 1);
		*hname = (char*) sqlite3_column_text(ppStmt, 2);
		*unit = (char*) sqlite3_column_text(ppStmt, 3);
		*type = (char*) sqlite3_column_text(ppStmt, 4);
		retval = 1;
		break;
	case SQLITE_DONE:
		retval = 0;
		break;
	case SQLITE_BUSY:
	case SQLITE_ERROR:
	case SQLITE_MISUSE:
	default:
		retval = -1;
		break;
	}

	return retval;
}

int logdb_etype_list_end(sqlite3_stmt *ppStmt){
	return logdb_list_end(ppStmt);  
}

int logdb_etype_count(struct logdb *db) {
	char sql[DB_QUERY_MAX];
	int ptr = 0;
	int result = 0;

	ptr += snprintf(sql + ptr, DB_QUERY_MAX - ptr, "SELECT Count(*) FROM %s ",
			DB_TABLE_NAME_EVENT_TYPE);

	if (logdb_get_first_int(db, sql, ptr, &result) != 0) {
		return -EFAULT;
	}

	return result;
}

int logdb_setting_set(struct logdb *db, const char *name, const char *value) {
	char sql[DB_QUERY_MAX];

	snprintf(sql, DB_QUERY_MAX, "REPLACE INTO settings (ename ,value) "
		"VALUES ('%s', '%s')", name, value);

	//    fprintf(stderr,"query %s\n", query);

	return logdb_sql(db, sql, NULL );
}

char *logdb_setting_get(struct logdb *db, const char *name, char* value,
		int len) {
	char sql[DB_QUERY_MAX];
	int ptr = 0;
	sqlite3_stmt *stathandel;
	const char *pzTail;
	char *retval = NULL;
	int sqlret;
	ptr += snprintf(sql, DB_QUERY_MAX, "SELECT value FROM settings "
		"WHERE ename='%s'", name);

	PRINT_DBG(db->debug_level, "Get setting, sql: %s", sql);

	sqlite3_prepare_v2(db->db, sql, ptr, &stathandel, &pzTail);

	sqlret = sqlite3_step(stathandel);

	switch (sqlret) {
	case SQLITE_ROW:
		if (value) {
			strncpy(value, (const char*) sqlite3_column_text(stathandel, 0),
					len);
			retval = value;
		} else
			retval = strdup((const char*) sqlite3_column_text(stathandel, 0));
		PRINT_DBG(db->debug_level, "Setting is %s", retval)
		;
		break;
	case SQLITE_DONE:
		PRINT_DBG(db->debug_level, "Get setting not found")
		;
		break;
	case SQLITE_BUSY:
	case SQLITE_ERROR:
	case SQLITE_MISUSE:
	default:
		break;
	}

	sqlite3_finalize(stathandel);

	PRINT_DBG(db->debug_level, "Setting returned %s", retval);
	return retval;
}

int logdb_check_integrity(struct logdb *db) {
	char sql[DB_QUERY_MAX];
	int ptr = 0;
	sqlite3_stmt *stathandel;
	const char *pzTail;
	int retval = 0;
	int sqlret;

	db->show_progress = 1;

	ptr += snprintf(sql, DB_QUERY_MAX, "PRAGMA integrity_check;");

	PRINT_DBG(db->debug_level, "To check, sql: %s", sql);

	sqlite3_prepare_v2(db->db, sql, ptr, &stathandel, &pzTail);

	sqlret = sqlite3_step(stathandel);

	while ((sqlret = sqlite3_step(stathandel)) == SQLITE_ROW) {
		const char* line = (const char*) sqlite3_column_text(stathandel, 0);
		if (strcmp("ok", line) != 0) {
			printf("%s\n", line);
			retval = -1;
		}
	}

	switch (sqlret) {
	case SQLITE_DONE:
		break;
	case SQLITE_BUSY:
	case SQLITE_ERROR:
	case SQLITE_MISUSE:
	default:
		retval = -1;
		break;
	}

	sqlite3_finalize(stathandel);

	db->show_progress = 0;

	PRINT_DBG(db->debug_level, "integrity_check returned %d", retval);
	return retval;

}

int logdb_delete(struct logdb *db, const char *table, const char *where) {
	char sql[DB_QUERY_MAX];
	int ptr = 0;
	int retval = 0;
	sqlite3_stmt *stathandel;
	const char *pzTail;
	int sqlret;

	ptr += snprintf(sql, DB_QUERY_MAX, "DELETE FROM %s WHERE %s", table, where);
	db->show_progress = 1;

	PRINT_DBG(db->debug_level, "Delete from  %s: ", sql);

	sqlite3_prepare_v2(db->db, sql, ptr, &stathandel, &pzTail);

	sqlret = sqlite3_step(stathandel);

	while ((sqlret = sqlite3_step(stathandel)) == SQLITE_ROW) {
		const char* line = (const char*) sqlite3_column_text(stathandel, 0);
		printf("DELETE: %s\n", line);
	}

	switch (sqlret) {
	case SQLITE_DONE:
		break;
	case SQLITE_BUSY:
	case SQLITE_ERROR:
	case SQLITE_MISUSE:
	default:
		retval = -1;
		break;
	}

	sqlite3_finalize(stathandel);

	db->show_progress = 0;
	return retval;
}


int logdb_evt_remove_sent(struct logdb *db){

	fprintf(stderr, "DELETING: time < lastsend \n");
	return logdb_delete(db, DB_TABLE_NAME_EVENT_LOG, " time < (select lastsend from event_type where event_log.eid = event_type.eid) ");	
}


int logdb_evt_remove_old(struct logdb *db, time_t oldtime) {
	char sql[DB_QUERY_MAX];
	int ptr;


	ptr += snprintf(sql, DB_QUERY_MAX, "time <= %ld", oldtime);

	fprintf(stderr, "DELETING: %s\n", sql);
	return logdb_delete(db, DB_TABLE_NAME_EVENT_LOG, sql);
}



int logdb_evt_count_max(struct logdb *db) {

	char vstr[32];
	char *ret;
	int count_max = 0;

	ret = logdb_setting_get(db, "evt_count_max", vstr, sizeof(vstr));

	if (ret) {
		if(strcmp(ret, "cacheonly")==0)
			return DB_SAVE_CACHEONLY;
		
		count_max = atoi(ret);
		if (count_max > 10000) 
			return count_max;
	
	}

	return DEFAULT_EVT_MAX;
}

int logdb_etype_removeobs(struct logdb *db) {

	int eid_rem[1024];
	char sql[DB_QUERY_MAX];
	int i = 0;
	int ptr = 0;
	sqlite3_stmt *stathandel;
	const char *pzTail;
	int sqlret, retval;

	time_t now    = time(NULL);
	time_t minday = now-(24*60*60*2);

	memset(eid_rem, 0, sizeof(eid_rem));

	ptr	+= snprintf(sql,
					DB_QUERY_MAX,
					"SELECT eid, (select COUNT(*) from event_log WHERE"
					" event_log.eid=event_type.eid) , lastlog"
					" AS count FROM event_type GROUP BY eid");

	db->show_progress = 1;

	PRINT_DBG(db->debug_level, "Count logs: %s: ", sql);

	sqlite3_prepare_v2(db->db, sql, ptr, &stathandel, &pzTail);

	sqlret = sqlite3_step(stathandel);

	while ((sqlret = sqlite3_step(stathandel)) == SQLITE_ROW) {
		int eid = sqlite3_column_int(stathandel, 0);
		int count = sqlite3_column_int(stathandel, 1);
		int lastlog = sqlite3_column_int(stathandel, 2);
		char where[64];
		if ((count == 0) && (lastlog < minday)) {
			sprintf(where, "eid=%d ", eid);
			printf("deleting from etypes where %s\n", where);
			logdb_delete(db, DB_TABLE_NAME_EVENT_TYPE, where);
		} else {
			printf("etypes %d has %d rows (lastlog %d sec ago)\n", eid, count, now-lastlog);
		}

	}

	switch (sqlret) {
	case SQLITE_DONE:
		break;
	case SQLITE_BUSY:
	case SQLITE_ERROR:
	case SQLITE_MISUSE:
	default:
		retval = -1;
		break;
	}

	sqlite3_finalize(stathandel);

	db->show_progress = 0;

	return retval;

}

int logdb_evt_maintain(struct logdb *db) {
	int mintime = 0;
	int row_count;
	int row_max;

	db->show_progress = 1;
	row_count = logdb_evt_count(db, -1);
	row_max = logdb_evt_count_max(db);
	db->show_progress = 0;

	printf("Count %d, count max %d\n", row_count, row_max);

	if(row_max == DB_SAVE_CACHEONLY){
		logdb_evt_remove_sent(db);
	} else if (row_count > row_max) {
		logdb_get_first_int(db, "SELECT MIN(time) FROM event_log", 0, &mintime);
		mintime += 60 * 60 * 24;
	} 

	if(mintime){
		logdb_evt_remove_old(db, mintime);
		logdb_etype_removeobs(db);
	}
	


	return 0;

}

int logdb_evt_vacuum(struct logdb *db) 
{
	logdb_sql(db, "VACUUM;", 0);	
}



int logdb_lock(struct logdb *db)
{
	int retval = 0;
    int timeout = 15;
	
    while(timeout > 0){
		PRINT_DBG(db->debug_level, "BEGIN EXCLUSIVE (%d)\n", timeout);
		retval = logdb_sql(db,"BEGIN EXCLUSIVE", NULL);
		if(retval == SQLITE_OK)
			return SQLITE_OK;

		sleep(1);
		timeout--;
    }
    fprintf(stderr, "BEGIN EXCLUSIVE timed out %d %d\n", retval, timeout);
    return SQLITE_BUSY;

}

int logdb_unlock(struct logdb *db)
{
	int retval = 0;
    int timeout = 15;

	while(timeout > 0){
		retval = logdb_sql(db,"COMMIT", NULL);
		if(retval == SQLITE_OK)
			return SQLITE_OK;
		sleep(1);
		timeout--;
	}

	return retval;
}

