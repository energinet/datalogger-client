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
 
#include "cmddb.h"

#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <logdb.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/ioctl.h>
//#include <linux/in.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <sqlite3.h>
#include <stdlib.h>


#define DB_CREATE_TABLE_CMD "CREATE TABLE IF NOT EXISTS"

#define DB_TABLE_NAME_CMD_LST "cmd_list"
#define DB_TABLE_COLU_CMD_LST				\
    "( cid INTEGER UNIQUE, name TEXT, value TEXT," \
    " ttime INTEGER, status INTEGER, stime INTEGER, retval TEXT, sent INTEGER, pseq INTEGER )"


struct sqlite3 *cdb = NULL;
int dbglev = 0;

struct cmddb_cmd *cmddb_cmd_create(int cid, const char *name, const char* value, int status, 
				    time_t stime, time_t ttime, const char *retval)
{
    struct cmddb_cmd *new = malloc(sizeof(*new));
    
    assert(new);
    memset(new, 0, sizeof(*new));
    
    assert(name);
    assert(value);

    new->cid = cid;
    new->name = strdup(name);
    new->value = strdup(value);
    new->status = status;
    new->stime = stime;
    new->ttime = ttime;
    new->next  = NULL;
    
    if(retval)
	new->retval = strdup(retval);
    else 
	new->retval = strdup("");
    
    return new;
}

struct cmddb_cmd *cmddb_cmd_add(struct cmddb_cmd *list, struct cmddb_cmd *new)
{
    struct cmddb_cmd *ptr = list;

    if(!list)
	return new;
  
    while(ptr->next)
	ptr = ptr->next;

    ptr->next = new;

    return list;
}

void cmddb_cmd_delete(struct cmddb_cmd *cmd)
{
    if(!cmd)
	return;
    
    cmddb_cmd_delete(cmd->next);
    
    free(cmd->name);
    free(cmd->value);
    free(cmd->retval);
    
    return;
}

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

int cmddb_query__(struct sqlite3 *db, const char *query, int len)
{
    int retval;
    sqlite3_stmt *stathandel;
    const char *pzTail;
    

    assert(query);

    if(len==0)
	len = strlen(query);
    
    retval = sqlite3_prepare_v2(db, query,  len , &stathandel, &pzTail );
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\"\n", retval, query);
	return -EFAULT;
    }

    retval =  sqlite3_step(stathandel);

    switch(retval){
      case SQLITE_DONE:
	retval = SQLITE_OK;
	if(dbglev)fprintf(stderr, "Info: Query done ok!! \"%s\"\n", query);
	break;
      case SQLITE_ROW:
	if(dbglev)fprintf(stderr, "Info: Query returned row!! \"%s\"\n", query);
	retval = SQLITE_OK;
	break;
      case SQLITE_BUSY:
      case SQLITE_ERROR:
      case SQLITE_MISUSE:
      default:
	if(dbglev)fprintf(stderr, "Error: Query faulted !! \"%s\"\n", query);
	retval = -EFAULT;
        break;
    }

    sqlite3_finalize(stathandel);

    return retval;

}


int cmddb_query(struct sqlite3 *db, const char *query, int len)
{
    cmddb_query__(db, query, len);
}

/**
 * Init command database 
 */
int cmddb_db_init(struct sqlite3 *db)
{
    int retval;
    
    retval = cmddb_query(db, 
			    DB_CREATE_TABLE_CMD " " DB_TABLE_NAME_CMD_LST " " DB_TABLE_COLU_CMD_LST,0);
    if (retval != 0){
        fprintf(stderr, "Error creating table %s", DB_TABLE_NAME_CMD_LST);
        return retval;
    }

    return SQLITE_OK;
}


/**
 * Open command database
 */
int cmddb_db_open(const char *path, int dbglev_)
{
    int retval;
    
    dbglev = dbglev_;

    if(!path){
	path = DEFAULT_CMD_LIST_PATH;
	if(dbglev)fprintf(stderr, "Default path: %s\n", DEFAULT_CMD_LIST_PATH);
    }

    if(dbglev)fprintf(stderr, "Opening database file %s\n", path);

    retval = sqlite3_open(path, &cdb);

    if(retval!=SQLITE_OK){
        fprintf(stderr,"sqlite3_open returned %d\n", retval);
        return retval;
    }

    if(cmddb_db_init(cdb)!=SQLITE_OK){
	goto err_out;
    }

    if((retval=sqlite3_busy_timeout(cdb, 30000))!=SQLITE_OK)
	fprintf(stderr,"Error setting timeout (%d)\n", retval);



    if(dbglev){
	fprintf(stderr,"setting timeout returned (%d)\n", retval);
	fprintf(stderr,"sqlite3_threadsafe id %s\n", (sqlite3_threadsafe())?"true":"false");
    }
    

    return 0;

  err_out:
    sqlite3_close(cdb);

    return -EFAULT;
    
}


/**
 * Close command database
 */
int cmddb_db_close(void)
{
    if(!cdb)
	return;
    
    sqlite3_close(cdb);
    cdb = NULL;
}


int cmd_db_lock(void)
{
    
    int retval = 0;
    int timeout = 15;

    while(timeout > 0){
	retval = cmddb_query(cdb,"BEGIN IMMEDIATE", 0);
	if(retval == SQLITE_OK)
	    return SQLITE_OK;
//	fprintf(stderr, "BEGIN IMMEDIATE returned %d (timeout %d)\n", retval, timeout);
	sleep(1);
	timeout--;
    }
    fprintf(stderr, "BEGIN IMMEDIATE timed out %d %d\n", retval, timeout);
    return SQLITE_BUSY;
}

int cmd_db_unlock(void)
{
    return cmddb_query(cdb,"COMMIT", 0);
}


int cmddb_time_print(time_t ptime, char *buf, int maxlen)
{
    struct tm localtime;

    localtime_r(&ptime, &localtime);

    return snprintf(buf, maxlen, "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", 
		    localtime.tm_year+1900, localtime.tm_mon+1, localtime.tm_mday,
		    localtime.tm_hour , localtime.tm_min  ,localtime.tm_sec);
}



int cmddb_db_print(void)
{
    int retval;
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len;
    
    len = sprintf(query, "SELECT cid, name, value, ttime, status, stime, retval, sent, pseq "
		  "FROM "DB_TABLE_NAME_CMD_LST);

    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
	return -EFAULT;
    }

    printf("%4s , %-20s , %-20s , %-6s , %-19s , %-19s , %-4s , %4s , %-30s\n", 
	   "cid", "name", "value" , "status", "trigger time", "status time", "sent", "dep", "retval");

    while((retval =  sqlite3_step(stathandel)) == SQLITE_ROW){
	char time_str[64];
	char stime_str[64];
	cmddb_time_print((time_t)sqlite3_column_int(stathandel, 3), time_str, sizeof(time_str));
	cmddb_time_print((time_t)sqlite3_column_int(stathandel, 5), stime_str, sizeof(stime_str));
	printf("%4d , %-20s , %-20s , %6d , %-19s , %-19s , %-4d , %4d , %-30s\n", 
	       sqlite3_column_int(stathandel, 0),
	       sqlite3_column_text(stathandel, 1),
	       sqlite3_column_text(stathandel, 2),
	       sqlite3_column_int(stathandel, 4),
	       time_str, stime_str,
	       sqlite3_column_int(stathandel, 7),
	       sqlite3_column_int(stathandel, 8),
	       sqlite3_column_text(stathandel, 6));
    }

    switch(retval){
      case SQLITE_DONE:
	retval = SQLITE_OK;
	break;
      case SQLITE_BUSY:
      case SQLITE_ERROR:
      case SQLITE_MISUSE:
      default:
	retval = -EFAULT;
        break;
    }

    sqlite3_finalize(stathandel);

    return retval;
}

/**
 * generate a local command id
 */
int cdmdb_cid_genlocal(void)
{
     sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int retval = -EFAULT;
    int len = 0;
    int cid = 0;
    
    len += snprintf(query+len, sizeof(query)-len, "SELECT MIN(cid) FROM cmd_list");
    
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    retval =  sqlite3_step(stathandel);

    switch(retval){
      case SQLITE_DONE:
	retval = 0;
	break;
      case SQLITE_ROW:
	cid = sqlite3_column_int(stathandel, 0);
	retval = 0;
	break;
      case SQLITE_BUSY:
      case SQLITE_ERROR:
      case SQLITE_MISUSE:
      default:
	retval = -EFAULT;
        break;
    }

    sqlite3_finalize(stathandel);

    if(cid < 0)
	cid--;
    else 
	cid = -1;

    return cid;   
    
}

/**
 * Insert a command into the db
 * @param cid   : command id
 * @param name  : command name
 * @param value : command param/set value
 * @param ttime : command trigger time
 * @param stime : status time (now)
 * @param pseq  : previous command in a sequence 
 */
int cmddb_insert(int cid, const char *name, const char *value, 
		 time_t ttime, int pseq, time_t now)
{
    char query[512];
    int len = 0;
    int status =  CMDSTA_QUEUED;
    time_t lastexec = cmddb_time_execed(name, now);
    
    if(!cid)
		cid = cdmdb_cid_genlocal();

    if(ttime == 0)
		ttime = now;
    else if(lastexec >= ttime){
		status = CMDSTA_ERROBS;
		fprintf(stderr, "cmd \"%s\" ttime is obsolete %lu sec\n", name,  lastexec - now);
    }
    
    
    len = sprintf(query, "INSERT OR REPLACE INTO "DB_TABLE_NAME_CMD_LST
		  " ( cid, name, value , status, ttime, stime, sent, pseq)"
		  " VALUES ( %d, '%s', '%s', %d, %lu, %lu, 0, %d)", 
		  cid, name, value, status , ttime, now,  pseq);

    return cmddb_query(cdb, query, len);
}

/**
 * Remove a command from the list
 */
int cmddb_remove(int cid)
{
    char query[512];
    int len = 0;

    len = sprintf(query, "DELETE FROM "DB_TABLE_NAME_CMD_LST
		  " WHERE cid=%d",  cid);

    return cmddb_query(cdb, query, len);
}

int cmddb_mark_remove_list(const char *list, const char *colid)
{
	char query[1024];
    int len = 0;



    len = sprintf(query, "UPDATE "DB_TABLE_NAME_CMD_LST
				  " SET sent=-1 , status = %d WHERE %s IN (%s) AND status = %d "
				  , CMDSTA_DELETED, colid, list, CMDSTA_QUEUED);
	
    return cmddb_query(cdb, query, len);
}


/**
 * Remove a command from the list
 */
int cmddb_mark_remove(int cid)
{
	char list[32];
	sprintf(list, "%d", cid);

	return cmddb_mark_remove_list(list, "cid");
}


/**
 * Set command status 
 * @param id     : command id
 * @param col    : id column
 * @param status : new status 
 * @param remsent: remove entry when send 
 * @param retval : pointer to the return value text to write
 * @param now    : time (now or a point in time)
 */
int cmddb_status_set(int id, const char *col, int status, char* retval, int remsent, time_t now) 
{
    char query[512];
    int len = 0;
    int sent = 0;

    if(remsent)
	sent = -1;
    
    len += snprintf(query+len, sizeof(query)-len, "UPDATE "DB_TABLE_NAME_CMD_LST
		    " SET status=%d, stime=%lu, sent=%d", status, now, 0);

    if(retval){
	len += snprintf(query+len, sizeof(query)-len, ", retval='%s'", retval);
	printf("retval %s\n", retval);
    }
    
    len += snprintf(query+len, sizeof(query)-len, " WHERE %s=%d", col, id);
    
    printf("update status ---> %s\n", query);

    return cmddb_query(cdb, query, len);
}

/**
 * Get the status for a given command id
 * @param cid    : command id
 * @param now    : time (now or a point in time)
 */
int cmddb_status_get(int cid)
{
  int retval =  CMDSTA_UNKNOWN;
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len;

    len = snprintf(query, sizeof(query),
		   "SELECT status FROM "DB_TABLE_NAME_CMD_LST" WHERE cid='%d'", cid );
    
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
	return 0;
    }

    if(sqlite3_step(stathandel) == SQLITE_ROW){
	retval = sqlite3_column_int(stathandel, 0);    
    }
	
    sqlite3_finalize(stathandel);

    return retval;

}


int cmddb_cmd_exec(struct cmddb_cmd *cmd, struct cmddb_desc *desc,  void *sessdata, unsigned long rowid, time_t now)
{
    int status = CMDSTA_EXECUTING;
    char *retval = NULL;
    time_t t_start = time(NULL);
    int remsent = 0;

    assert(cmd);
    
    //  cmddb_status_set(cmd->cid, status, retval,0   , now);
    cmddb_status_set(rowid, "rowid", status, retval,0   , now);

    if(desc){
	if(desc->handler)
	    status =  desc->handler(cmd, desc->userdata, sessdata, &retval);
	else
	    status = CMDSTA_EXECUTED;
	if(!(CMDFLA_KEEPCURR&desc->flags)||(status != CMDSTA_EXECUTED)) 
	    remsent = 1;
    } else {
	status = CMDSTA_UNKNOWN;
    }


    cmddb_status_set(rowid, "rowid", status, retval, remsent , now-(time(NULL)-t_start));
//    cmddb_status_set(cmd->cid, "rowid", status, retval, remsent , now-(time(NULL)-t_start));

    if(status == CMDSTA_EXECUTED)
		cmddb_mark_prev(cmd->name, cmd->ttime);
	else 
		cmddb_mark_error(rowid, "rowid");


    return 0;
    
}


/**
 * Get commands to execute
 * @param list : list of enabled commands 
 *
 */

int cmddb_exec_list(struct cmddb_desc *list, void *sessdata, time_t now)
{
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len=0, retval=0;
    int count = 0;
    struct cmddb_cmd *execcmd = NULL;
    struct cmddb_desc *ptr = list;
    
    
    if(cmd_db_lock() != SQLITE_OK){
	fprintf(stderr, "DB is busy\n");
	return 0;
    }




    len += snprintf(query+len, sizeof(query)-len,
		    "SELECT cid, name, value, status, ttime, pseq, rowid FROM "DB_TABLE_NAME_CMD_LST" WHERE status=%d "
		    "AND ttime<=%lu AND name IN (", CMDSTA_QUEUED, now);
    
    
    
    while(ptr){
	len += snprintf(query+len, sizeof(query)-len,
			"'%s'", ptr->name);
	if(ptr->next)
	    len += snprintf(query+len, sizeof(query)-len,
			    ",");
	ptr = ptr->next;
    }
    
    len += snprintf(query+len, sizeof(query)-len,
		    ") ORDER BY ttime ASC");


    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
	retval = -EFAULT;
	goto out;
    }

    while(sqlite3_step(stathandel) == SQLITE_ROW){
	int pseq = sqlite3_column_int(stathandel, 5);
	int cid = sqlite3_column_int(stathandel, 0);
	const char *name = (const char*)sqlite3_column_text(stathandel, 1);
	struct cmddb_desc *desc = cmddb_desc_get(list,name);
	
	fprintf(stderr, "cid %d dep on %d\n", cid, pseq);
	if(pseq){
	    if(cmddb_status_get(pseq)!=CMDSTA_EXECUTED){
		
		continue;
	    }
	}

	execcmd = cmddb_cmd_create(sqlite3_column_int(stathandel, 0),  /*cid*/ 
				   (const char*)sqlite3_column_text(stathandel, 1), /*name*/
				   (const char*)sqlite3_column_text(stathandel, 2), /*value*/
				   sqlite3_column_int(stathandel, 3),  /*status*/
				   0 /*stime*/,sqlite3_column_int(stathandel, 4),
				   NULL/* retval*/);

	cmddb_cmd_exec(execcmd, desc,sessdata,  sqlite3_column_int(stathandel, 6), now);
	count++;

	cmddb_cmd_delete(execcmd);

    }
	
    sqlite3_finalize(stathandel);


  out:
    cmd_db_unlock();

    return count;
}



time_t cmddb_value(const char *name, char *buf , size_t maxsize, time_t now)
{
    
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len, retval;
    time_t ttime = 0;

    len = snprintf(query, sizeof(query),
		   "SELECT value,ttime FROM "DB_TABLE_NAME_CMD_LST" WHERE name='%s' "
		   "AND ttime<=%lu ORDER BY ttime DESC", name, now);
    
    
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
	return 0;
    }

    if(sqlite3_step(stathandel) == SQLITE_ROW){
	const char *retptr = (const char*) sqlite3_column_text(stathandel, 0);
	if(retptr)
	    strncpy(buf, retptr, maxsize);
	ttime =  sqlite3_column_int(stathandel, 1);
    }
	
    sqlite3_finalize(stathandel); 

    return ttime;
}

time_t cmddb_time_execed(const char *name, time_t now)
{

    time_t retval = 0;
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len;

    len = snprintf(query, sizeof(query),
		   "SELECT ttime FROM "DB_TABLE_NAME_CMD_LST" WHERE name='%s' AND status=%d "
		   "AND ttime<=%lu ORDER BY ttime DESC", name, CMDSTA_EXECUTED, now);
    
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
	return 0;
    }

    if(sqlite3_step(stathandel) == SQLITE_ROW){
	retval = sqlite3_column_int(stathandel, 0);    
    }
	
    sqlite3_finalize(stathandel);

    return retval;
    
}

int cmddb_sent_mark(int cid, int status)
{
    char query[512];
    int len = 0;

    /* mark as sent */
    len += snprintf(query+len, sizeof(query)-len, "UPDATE "DB_TABLE_NAME_CMD_LST
		    " SET sent=(sent+10) WHERE cid=%d AND status=%d ", cid, status );

    printf("mark sent %s\n",  query);
    
    return cmddb_query(cdb, query, len);
}

int cmddb_mark_error(int id, const char *col)
{
	char query[512];
    int len = 0;

	len += snprintf(query+len , sizeof(query)-len, 
					"UPDATE "DB_TABLE_NAME_CMD_LST" SET sent = sent-1 WHERE sent IN (0,10)");

	len += snprintf(query+len, sizeof(query)-len, " AND %s=%d", col, id);
   
	return cmddb_query(cdb, query, len);
}

int cmddb_mark_prev(char *name, time_t ttime )
{
    char query[512];
    int len = 0;
    
    if(dbglev)fprintf(stderr, "boxvcmd_mark_prev: %s, %lu\n", name, ttime);

    len += snprintf(query+len , sizeof(query)-len, 
		    "UPDATE "DB_TABLE_NAME_CMD_LST" SET sent = sent-1 WHERE sent IN (0,10)");

    
    len += snprintf(query+len , sizeof(query)-len, 
		    " AND status != %d AND name='%s'" , CMDSTA_EXECUTING, name );

    len += snprintf(query+len , sizeof(query)-len, 
		    " AND ttime < %lu", ttime);
    
    if(dbglev)fprintf(stderr, "boxvcmd_mark_prev: %s\n", query);

    return cmddb_query(cdb, query, len);

}



int cmddb_clean(const char *cond)
{
      sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[1024];
    int len = 0, retval, count = 0;
    struct cmddb_cmd *updlst = NULL;
    //   dbglev = 1;

    if(cmd_db_lock() != SQLITE_OK){
	fprintf(stderr, "DB is busy\n");
	return 0;
    }


    if(dbglev)fprintf(stderr, "deleting sends \n");
    len += snprintf(query+len, sizeof(query)-len, 
		   "SELECT rowid FROM "DB_TABLE_NAME_CMD_LST" B WHERE %s"
		    " AND (SELECT COUNT() FROM "DB_TABLE_NAME_CMD_LST" A"
		    " WHERE A.pseq = B.cid) = 0 LIMIT 100 ", cond);


    if(dbglev)fprintf(stderr,"sql %s\n", query);
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
	retval = -EFAULT;
	goto out;
    }
    len = 0;
    len += snprintf(query+len, sizeof(query)-len, 
		   "DELETE FROM "DB_TABLE_NAME_CMD_LST" WHERE rowid IN (");

    while(sqlite3_step(stathandel) == SQLITE_ROW){
	if(count)
	    len += snprintf(query+len, sizeof(query)-len, ",");
	
	len += snprintf(query+len, sizeof(query)-len, "%d",sqlite3_column_int(stathandel, 0));
	count++;
    }
	
    len += snprintf(query+len, sizeof(query)-len, ")");

    if(dbglev)fprintf(stderr,"sql %s\n", query);
    sqlite3_finalize(stathandel);

    retval = cmddb_query(cdb, query, len);
    
  out:
    cmd_db_unlock();

    return retval;

}

int cmddb_sent_clean(void)
{
    return cmddb_clean("sent=(10-1)");
}


int cmddb_notsent_clean(void)
{
    return cmddb_clean("sent=(-1)");
}

/**
 * Get all updated status' 
 * All obsolete command/value updates will be removed
 * @retuen a list if commands/updates 
 */
struct cmddb_cmd *cmddb_get_updates(void)
{
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len, retval;
    struct cmddb_cmd *updlst = NULL;
    

    len = snprintf(query, sizeof(query),
		   "SELECT cid, name, value, status, stime, ttime, retval FROM "DB_TABLE_NAME_CMD_LST" WHERE "
		   "sent<=0 ORDER BY stime ASC");

    
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
	return NULL;
    }

    while(sqlite3_step(stathandel) == SQLITE_ROW){
	struct cmddb_cmd *new;
	new = cmddb_cmd_create(sqlite3_column_int(stathandel, 0),  /*cid*/ 
			       (const char*)sqlite3_column_text(stathandel, 1), /*name*/
			       (const char*)sqlite3_column_text(stathandel, 2), /*value*/
			       sqlite3_column_int(stathandel, 3),  /*status*/
			       sqlite3_column_int(stathandel, 4), /*stime*/
			       sqlite3_column_int(stathandel, 5), /*stime*/
			       (const char*)sqlite3_column_text(stathandel, 6)); /* retval*/
	
	updlst = cmddb_cmd_add(updlst, new);

    }
	
    sqlite3_finalize(stathandel);

    return updlst; 
    
}




int cmd_cid_change(int cid_old, int cid_new)
{
    char query[512];
    int len = 0;

    /* mark as sent */
    len += snprintf(query+len, sizeof(query)-len, "UPDATE "DB_TABLE_NAME_CMD_LST
		    " SET cid=%d WHERE cid=%d", cid_new, cid_old );
    
    fprintf(stderr, "change cid %d --> %d (%s)\n", cid_old, cid_new, query);

    return cmddb_query(cdb, query, len);
}




/***********************************
 * Command database plan functions */

struct cmddb_plan *cmddb_plan_create(int rowid, time_t ttime, const char *value)
{
	struct cmddb_plan *new = malloc(sizeof(*new));
	assert(new);

	new->rowid = rowid;
	new->ttime = ttime;
	
	if(value)
		new->value = strdup(value);
	else
		new->value = NULL;
	new->next = NULL;
	
	return new;
}


struct cmddb_plan *cmddb_plan_add(struct cmddb_plan *list ,struct cmddb_plan *new)
{
	struct cmddb_plan *ptr = list;

    if(!list)
        /* first module in the list */
        return new;
	
    /* find the last module in the list */
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}

struct cmddb_plan *cmddb_plan_rem(struct cmddb_plan *list ,struct cmddb_plan *rem)
{
	struct cmddb_plan *ptr = list;
	struct cmddb_plan *prev = NULL;

    /* find the last module in the list */
    while(ptr){
        if(rem == ptr){
            if(prev)
                prev->next = ptr->next;
            else
                list = ptr->next;
            
            ptr->next = NULL;
            break;
        }
        prev = ptr;
        ptr = ptr->next;
    }

    return list;

}





void cmddb_plan_delete(struct cmddb_plan *plan)
{
	if(!plan)
		return;
	
	cmddb_plan_delete(plan->next);

	free(plan->value);

	free(plan);
}


void cmddb_plan_print(struct cmddb_plan *plan)
{
	int count = 0;
	printf("plan: %p\n", plan);

	while(plan){
		printf("%p %2d: %5.5d  %s : %s", plan , count++, plan->rowid, plan->value, ctime(&plan->ttime));
		
		plan = plan->next;
	}

}

struct cmddb_plan *cmddb_plan_find(struct cmddb_plan *plan, time_t ttime)
{
	while(plan){
		/* fprintf(stderr, "plan%p %ld==%ld\n", plan, plan->ttime , ttime); */
		if(plan->ttime == ttime)
			return plan;

		plan = plan->next;
	}

	return NULL;
}


int cmddb_plan_prune(struct cmddb_plan **new_plan__, struct cmddb_plan **old_plan__)
{

	struct cmddb_plan *new_plan = *new_plan__;	
	struct cmddb_plan *old_plan = *old_plan__;
	struct cmddb_plan *ptr = new_plan;
	int pcount = 0;

	if(!old_plan)
		return 0;

	if(!new_plan)
		return 0;

	while(ptr){
		struct cmddb_plan *res = cmddb_plan_find(old_plan, ptr->ttime);

		if(res){
			if(strcmp(res->value, ptr->value)==0){
//				fprintf(stderr, "del: ptr(%p):%s, res(%p):%s \n", 
//						ptr,  ptr->value, res, res->value );
				struct cmddb_plan *rem = ptr;
				ptr = ptr->next;				
				old_plan = cmddb_plan_rem(old_plan , res);
				cmddb_plan_delete(res);
				new_plan = cmddb_plan_rem(new_plan , rem);
				cmddb_plan_delete(rem);
				pcount++;
		
				continue;
			} else {
				/* fprintf(stderr, "dif: ptr(%p):%s, res(%p):%s \n",  */
				/* 		ptr,  ptr->value,res, res->value ); */
			}
		} else {
				/* fprintf(stderr, "non: ptr(%p):%s,  \n",  */
				/* 		ptr, ptr->value ); */
		}

		ptr = ptr->next;
	}

	*new_plan__ = new_plan;
	*old_plan__ = old_plan;

	return pcount;
}



int cmddb_plan_set__(struct cmddb_plan *plan, const char *cmd_name)
{
	while(plan){
		cmddb_insert(0, cmd_name, plan->value, plan->ttime, 0, time(NULL));
		plan = plan->next;
	}
	
	return 0;
}

int cmddb_plan_rem__(struct cmddb_plan *plan, const char *cmd_name)
{
	char list[512] = "";
	int len = 0;
	int retval = 0;

	assert(cmd_name);
	

	while(plan){

		if(len >= (sizeof(list)-10)){
			fprintf(stderr, "Error : Upper limit removal \n", retval);
			retval = -1;
			break;
		}

		if(len)
			list[len++] = ',';
		
		len += snprintf(list+len, sizeof(list)-len, "%d", plan->rowid);
		
		plan = plan->next;
	}
		

	cmddb_mark_remove_list(list, "rowid");

	return retval;
}




/**
 * Write a plan to the database
 */
int cmddb_plan_set(struct cmddb_plan *plan, const char *cmd_name, time_t t_begin, time_t t_end)
{
	struct cmddb_plan *old_plan = NULL;
	int retval = 0;
	
	assert(cmd_name);

	if(t_begin&&t_end){
		old_plan = cmddb_plan_get(cmd_name, t_begin, t_end);
		cmddb_plan_prune(&plan, &old_plan);
	} 

	if(dbglev){
		cmddb_plan_print(plan);
		cmddb_plan_print(old_plan);
	}
	
	if(retval = cmddb_plan_set__(plan, cmd_name)<0){
		fprintf(stderr, "Error %d: Could not set new plan \n", retval);
		return retval;
	}		
	

	if(retval = cmddb_plan_rem__(old_plan, cmd_name)<0){
		fprintf(stderr, "Error %d: Could not set new plan \n", retval);
		return retval;
	}

	cmddb_plan_delete(plan);
	cmddb_plan_delete(old_plan);
	

	return 0;

}


struct cmddb_plan *cmddb_plan_get(const char *cmd_name, time_t t_begin, time_t t_end)
{
	sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len, retval;
    struct cmddb_plan *list = NULL;

	len = snprintf(query, sizeof(query),
				   "SELECT rowid, ttime, value FROM "DB_TABLE_NAME_CMD_LST" WHERE "
				   "ttime >= %ld AND ttime <= %ld AND name = '%s' AND status = 1 ORDER BY ttime ASC", 
				   t_begin, t_end, cmd_name);

    
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
		fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
		return NULL;
    }
	
    while(sqlite3_step(stathandel) == SQLITE_ROW){
		struct cmddb_plan *new;
		new = cmddb_plan_create(sqlite3_column_int(stathandel, 0), /* rowid */
 								sqlite3_column_int(stathandel, 1), /* ttime */ 
								(const char*)sqlite3_column_text(stathandel, 2) /*value*/);

		list = cmddb_plan_add(list, new);

    }
	
    sqlite3_finalize(stathandel);

    return list; 
    
}

