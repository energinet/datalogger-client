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

#include "boxcmdq.h"

#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <logdb.h>
#include <linux/if.h>
#include <sys/ioctl.h>
//#include <linux/in.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include "soapH.h" // obtain the generated stub
#include "rpserver.h"


#define DB_CREATE_TABLE_CMD "CREATE TABLE IF NOT EXISTS"

#define DB_TABLE_NAME_CMD_LST "cmd_list"
#define DB_TABLE_COLU_CMD_LST				\
    "( cid INTEGER PRIMARY KEY, name TEXT, value TEXT," \
    " ttime INTEGER, status INTEGER, stime INTEGER, retval TEXT, sent INTEGER, pseq INTEGER )"


struct sqlite3 *cdb = NULL;


struct boxvcmd *boxvcmd_create(int cid, const char *name, const char* value, int status, 
			       time_t stime, const char *retval)
{
    struct boxvcmd *new = malloc(sizeof(*new));
    
    assert(new);
    memset(new, 0, sizeof(*new));
    
    assert(name);
    assert(value);

    new->cid = cid;
    new->name = strdup(name);
    new->value = strdup(value);
    new->status = status;
    new->stime.tv_sec = stime;
    
    if(retval)
	new->retval = strdup(retval);
    else 
	new->retval = strdup("");
    return new;
}



struct boxvcmd *boxvcmd_add(struct boxvcmd *list, struct boxvcmd *new)
{
    struct boxvcmd *ptr = list;

    if(!list)
	return new;
  
    while(ptr->next)
	ptr = ptr->next;

    ptr->next = new;

    return list;

}


/**
 * Delete boxvcmd
 */
void boxvcmd_delete(struct boxvcmd *cmd)
{
    if(!cmd)
	return;
    
    boxvcmd_delete(cmd->next);

    free(cmd->name);
    free(cmd->value);
    free(cmd->retval);
}



char *boxvcmd_strdup(const unsigned char *text)
{
    if(!text)
	return strdup("");

    return strdup((char *)text);
    
}


int boxvcmd_sqlque(struct sqlite3 *db, const char *query, int len)
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
	break;
      case SQLITE_ROW:
	fprintf(stderr, "Info: Query returned row!! \"%s\"\n", query);
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



int boxvcmd_init(struct sqlite3 *db)
{
    int retval;
    
    retval = boxvcmd_sqlque(db, 
			    DB_CREATE_TABLE_CMD " " DB_TABLE_NAME_CMD_LST " " DB_TABLE_COLU_CMD_LST,0);
    if (retval != 0){
        fprintf(stderr, "Error creating table %s", DB_TABLE_NAME_CMD_LST);
        return retval;
    }

    return SQLITE_OK;
}


/**
 * Open the command/update list 
 */
int boxvcmd_open(void)
{
    int retval;
    
    fprintf(stderr, "Opening database file %s\n", DEFAULT_CMD_LIST_PATH);

    retval = sqlite3_open(DEFAULT_CMD_LIST_PATH, &cdb);

    if(retval!=SQLITE_OK){
        fprintf(stderr,"sqlite3_open returned %d\n", retval);
        return retval;
    }

    if(boxvcmd_init(cdb)!=SQLITE_OK){
	goto err_out;
    }

    if((retval=sqlite3_busy_timeout(cdb, 15000))!=SQLITE_OK)
	fprintf(stderr,"Error setting timeout (%d)\n", retval);

    return 0;

  err_out:
    sqlite3_close(cdb);

    return -EFAULT;
    
}


/**
 * Close the command/update list
 */
void boxvcmd_close(void)
{
    if(!cdb)
	return;
    
    sqlite3_close(cdb);
    cdb = NULL;
}


/**
 * Set command into the command list/queue
 */
int boxvcmd_queue(int cid, const char *name, const char *value, 
		  struct timeval *ttime, struct timeval *stime, int pseq)
{
    char query[512];
    int len = 0;
    int status =  CMDSTA_QUEUED;
    unsigned long lastupd = boxcmdq_value_updtime(name, stime);
    
    if(lastupd > ttime->tv_sec){
	status = CMDSTA_ERROBS;
	fprintf(stderr, "cmd \"%s\" ttime is obsolete %lu sec\n", name,  lastupd - ttime->tv_sec);
    } else
	fprintf(stderr, "cmd \"%s\" ttime is newer %lu sec (dep %d)\n", name,  ttime->tv_sec - lastupd, pseq);
    

    if(ttime->tv_sec == 0)
	memcpy(ttime, stime, sizeof(*stime));
    




    len = sprintf(query, "INSERT OR REPLACE INTO "DB_TABLE_NAME_CMD_LST
		  " ( cid, name, value , status, ttime, stime, sent, pseq)"
		  " VALUES ( %d, '%s', '%s', %d, %lu, %lu, 0, %d)", 
		  cid, name, value, status , 
		  (unsigned long)ttime->tv_sec, 
		  (unsigned long)stime->tv_sec, pseq);

    return boxvcmd_sqlque(cdb, query, len);
}


/**
 * Remove command from the list 
 */
int boxvcmd_remove(int cid)
{
    char query[512];
    int len = 0;

    len = sprintf(query, "DELETE FROM "DB_TABLE_NAME_CMD_LST
		  " WHERE cid=%d",  cid);

    return boxvcmd_sqlque(cdb, query, len);
}

int boxvcmd_remove_prev(int cid, int state)
{
    char query[512];
    int len = 0;

    len = sprintf(query, "DELETE FROM "DB_TABLE_NAME_CMD_LST
		  " WHERE sent=1 AND status=%d AND name=(SELECT name FROM "DB_TABLE_NAME_CMD_LST
		  " WHERE cid=%d) AND ttime<(SELECT ttime FROM "DB_TABLE_NAME_CMD_LST
		  " WHERE cid=%d)" , state, cid, cid);
    
    fprintf(stderr, "boxvcmd_remove_prev: %s\n", query);

    return boxvcmd_sqlque(cdb, query, len);

}

int boxvcmd_mark_sent(int cid)
{
    char query[512];
    int len = 0;
    /* mark as sent */
    len += snprintf(query+len, sizeof(query)-len, "UPDATE "DB_TABLE_NAME_CMD_LST
		    " SET sent=1 WHERE cid=%d", cid);
    
    return boxvcmd_sqlque(cdb, query, len);

}

int boxvcmd_set_sent(int cid, int status, int keep)
{   
    printf("boxvcmd_set_sent( %d, %d, %d)\n", cid,status,keep);

    boxvcmd_mark_sent(cid);

    switch(status){
      case CMDSTA_EXECUTING:
      case CMDSTA_QUEUED:
	boxvcmd_mark_sent(cid);
	break;
      case CMDSTA_EXECUTED:
	boxvcmd_remove_prev(cid,  CMDSTA_EXECUTED);
	if(!keep)
	    boxvcmd_remove(cid);
	break;
      case CMDSTA_DELETED:
      case CMDSTA_EXECERR:
      default:
	boxvcmd_remove(cid);
	break;

    }
    
    return 0;

}

/**
 * Change status of a specific command 
 */
int boxvcmd_status_set(int cid, int status, struct timeval *stime, char *retval)
{
    char query[512];
    int len = 0;

    
    len += snprintf(query+len, sizeof(query)-len, "UPDATE "DB_TABLE_NAME_CMD_LST
		   " SET status=%d, stime=%lu, sent=0", status, (unsigned long)stime->tv_sec);

    if(retval){
	len += snprintf(query+len, sizeof(query)-len, ", retval='%s'", retval);
	printf("retval %s\n", retval);
    }

    len += snprintf(query+len, sizeof(query)-len, " WHERE cid=%d", cid);

    printf("update status +--> %s\n", query);

    return boxvcmd_sqlque(cdb, query, len);
}

/**
 * Get the status command/update
 */
int boxvcmd_status_get(int cid, time_t *stime)
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

    return 0;
}


int boxvcmd_ptime(time_t time, char *buf, int maxlen)
{
    struct tm localtime;

    localtime_r(&time, &localtime);

    return snprintf(buf, maxlen, "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", 
		    localtime.tm_year+1900, localtime.tm_mon+1, localtime.tm_mday,
		    localtime.tm_hour , localtime.tm_min  ,localtime.tm_sec);
}


/**
 * Print the command list for debug purpose
 */
int boxcmdq_list_print(void)
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
	boxvcmd_ptime((time_t)sqlite3_column_int(stathandel, 3), time_str, sizeof(time_str));
	boxvcmd_ptime((time_t)sqlite3_column_int(stathandel, 5), stime_str, sizeof(stime_str));
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
 * Read the current value/paramater of a variable/command
 */
char* boxcmdq_value_current(const char *name, struct timeval *ttime , struct timeval *curtime)
{
    char *retptr = NULL;
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len, retval;

    len = snprintf(query, sizeof(query),
		   "SELECT value,ttime FROM "DB_TABLE_NAME_CMD_LST" WHERE name='%s' "
		   "AND ttime<=%lu ORDER BY ttime DESC", name, curtime->tv_sec);
    
    
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
	return NULL;
    }

    if(sqlite3_step(stathandel) == SQLITE_ROW){
	retptr = boxvcmd_strdup(sqlite3_column_text(stathandel, 0));
	if(ttime){
	    ttime->tv_sec = sqlite3_column_int(stathandel, 1);
	    ttime->tv_usec = 0;
	}
	
    }
	
    sqlite3_finalize(stathandel);

    

    return retptr;

}

/**
 * Read the current value/paramater of a variable/command
 */
unsigned long boxcmdq_value_updtime(const char *name, struct timeval *time)
{
    int retval = 0;
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len;

    len = snprintf(query, sizeof(query),
		   "SELECT ttime FROM "DB_TABLE_NAME_CMD_LST" WHERE name='%s' AND status=%d "
		   "AND ttime<=%lu ORDER BY ttime DESC", name, CMDSTA_EXECUTED, time->tv_sec);
    
    
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



int boxcmdq_list_print_values(struct timeval *time)
{
    return 0;
}



struct boxvcmd *boxvcmd_get_exec(struct timeval *time, unsigned long ignore )
{
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len, retval;
    struct boxvcmd *retcmd = NULL;

    
    len = snprintf(query, sizeof(query),
		   "SELECT cid, name, value, status, pseq FROM "DB_TABLE_NAME_CMD_LST" WHERE status=%d "
		   "AND ttime<=%lu ORDER BY ttime ASC", CMDSTA_QUEUED, time->tv_sec);

    
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
	return NULL;
    }

    while(sqlite3_step(stathandel) == SQLITE_ROW){
	int pseq = sqlite3_column_int(stathandel, 4);
	int cid = sqlite3_column_int(stathandel, 0);
	fprintf(stderr, "cid %d dep on %d\n", cid, pseq);
	if(pseq){
	    if(boxvcmd_status_get(pseq, NULL)!=CMDSTA_EXECUTED){

		continue;
	    }
	}


	if(boxcmd_get_flags((const char*)sqlite3_column_text(stathandel, 1))&ignore){
	    fprintf(stderr, "%s ignored\n", sqlite3_column_text(stathandel, 1));
	    continue;
	}

	retcmd = boxvcmd_create(sqlite3_column_int(stathandel, 0),  /*cid*/ 
				(const char*)sqlite3_column_text(stathandel, 1), /*name*/
				(const char*)sqlite3_column_text(stathandel, 2), /*value*/
 				sqlite3_column_int(stathandel, 3),  /*status*/
				0 /*stime*/,
				NULL/* retval*/);


    }
	
    sqlite3_finalize(stathandel);

    if(retcmd)
	boxvcmd_status_set(retcmd->cid, CMDSTA_EXECUTING, time, NULL);

    return retcmd;
}


/**
 * Get all updated status' 
 * All obsolete command/value updates will be removed
 * @param last the time the update was send 
 * @retuen a list if commands/updates 
 */
struct boxvcmd *boxvcmd_get_upd(struct timeval *last)
{
    sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len, retval;
    struct boxvcmd *updlst = NULL;
    
    len = snprintf(query, sizeof(query),
		   "SELECT cid, name, value, status, stime, retval FROM "DB_TABLE_NAME_CMD_LST" WHERE "
		   "sent=0 ORDER BY stime ASC");

    
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
	fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
	return NULL;
    }

    while(sqlite3_step(stathandel) == SQLITE_ROW){
	struct boxvcmd *new;
	new = boxvcmd_create(sqlite3_column_int(stathandel, 0),  /*cid*/ 
			     (const char*)sqlite3_column_text(stathandel, 1), /*name*/
			     (const char*)sqlite3_column_text(stathandel, 2), /*value*/
			     sqlite3_column_int(stathandel, 3),  /*status*/
			     sqlite3_column_int(stathandel, 4), /*stime*/
			     (const char*)sqlite3_column_text(stathandel, 5)); /* retval*/
	
	updlst = boxvcmd_add(updlst, new);

    }
	
    sqlite3_finalize(stathandel);

    return updlst; 
    
}
