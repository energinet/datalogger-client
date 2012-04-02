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

#include "soapH.h" 
#include "rpfchart.nsmap" 
#include <math.h> 
#include  <sqlite3.h>
#include <logdb.h>

#define PRINT_ERR(str,arg...) fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}


#ifdef DBDIR
#define DB_FILE DBDIR "/bigdb.sql"
#else
#define DB_FILE "/jffs2/bigdb.sql"
#endif

#define DB_QUERY_MAX 512
#define RPC_ETYPE_MAX 100
#define RPC_EVENT_MAX 20000

void rpftime(struct timeval *tv_start, const char *text)
{
	
	struct timeval tv_end;

	gettimeofday(&tv_end, NULL);

	tv_end.tv_sec -= tv_start->tv_sec;

	if(tv_end.tv_usec < tv_start->tv_usec){
		tv_end.tv_usec += 1000000;
		tv_end.tv_sec  += 1;
		tv_end.tv_usec -= tv_start->tv_usec;

	} else {
		tv_end.tv_usec -= tv_start->tv_usec;
	}

	fprintf(stderr, "Time diff at %s %ld.%6.6ld\n", text, tv_end.tv_sec, tv_end.tv_usec);
}


int main()  
{  
	
    struct soap *soap; 
    soap = soap_new1(SOAP_C_UTFSTRING|SOAP_IO_STORE); 
    soap_serve(soap); // call the incoming remote method request dispatcher 

    return 0;
} 

char *dg_strdup(const char *db_value)
{
    return strdup(db_value ? db_value : "NULL");
}

int db_callback_etype(void *data, int argc, char **argv, char **azColName)
{

    struct ns__etypes *etypes = (struct ns__etypes*) data;
    struct ns__etype  *etype  = etypes->__ptr + etypes->__size++;
    int i = 0;

    if( etypes->__size > RPC_ETYPE_MAX)
        return 0;

    etype->name = dg_strdup(argv[i++]);
    etype->unit = dg_strdup(argv[i++]);
    etype->description = dg_strdup(argv[i++]);
    etype->type = dg_strdup(argv[i++]);

    return 0;
    
}


int db_callback_event(void *data, int argc, char **argv, char **azColName)
{

    struct ns__events *events = (struct ns__events*) data;
    //struct ns__event  *event  = events->__ptr + events->__size++;
    int i = 0;

    if( events->__size > RPC_EVENT_MAX )
        return 0;

    //event->time = dg_strdup(argv[i++]);
    //event->value = dg_strdup(argv[i++]);
 
    return 0;
    
}


int db_get_rows( sqlite3 *db,  int t_start, float t_end, int* row_start, int *row_end)
{
    char quote[128]; 
    sqlite3_stmt *stathandel;
    const char *pzTail;
    int retval;
    int rowid_prev = 0;
    int i = 0;

    snprintf(quote, 128, "SELECT rowid, time FROM event_log");   

    retval = sqlite3_prepare_v2(db, quote,  strlen(quote), &stathandel, &pzTail );

    if(retval != SQLITE_OK){
        PRINT_ERR("retval != SQLITE_OK, retval = %d\n", retval);
        return SOAP_ERR;
    }
    
    while(1){
        retval =  sqlite3_step(stathandel);
        if(retval != SQLITE_ROW)
            break;
        
        if(i%100){
            i++;
            continue;
        }

        if(sqlite3_column_int(stathandel, 1) > t_start)
            break;

        rowid_prev = sqlite3_column_int(stathandel, 0);
        
        i++;
    } 

    *row_start = rowid_prev;

    
    while(1){
        retval =  sqlite3_step(stathandel);
        if(retval != SQLITE_ROW)
            break;
        
        if(i%100){
            i++;
            continue;
        }

        rowid_prev = sqlite3_column_int(stathandel, 0);

        if(sqlite3_column_int(stathandel, 1) > t_end)
            break;

        
        i++;
    } 

    PRINT_DBG(0, "i %d, ret %d", i, retval);

    if(i%100)
        *row_end = 0x7fffffff;
    else
        *row_end = rowid_prev;

    sqlite3_finalize(stathandel);

    return retval;


}






int db_run_query(const char *query, int (*callback)(void *data, int argc, char **argv, char **azColName), void *data)
{
    sqlite3 *db;
    int retval = -EFAULT;
    char *zErrMsg = NULL;
     
    retval = sqlite3_open(DB_FILE, &db);

    if(retval){
        PRINT_ERR("sqlite3_open returned %d", retval);
        sqlite3_close(db);
        return retval;
    }

    retval = sqlite3_exec(db, query, callback, data, &zErrMsg);

    if(retval != SQLITE_OK) {
        PRINT_ERR("SQL error: %s", zErrMsg);
        PRINT_ERR("SQL query: %s", query);
        sqlite3_free(zErrMsg);
        retval = -EFAULT;
    } else {
        retval = 0;
    }
    
    
    sqlite3_close(db);
       
    return retval;
}




int ns__getETypes(struct soap *soap, char *box, struct ns__etypes *result)
{
    char query[DB_QUERY_MAX];
    int retval;
    struct logdb *db = NULL;
    sqlite3_stmt *ppStmt = NULL;

    result->__ptr = malloc(sizeof(struct ns__etype)*RPC_ETYPE_MAX); 
    result->__size = 0; 
    result->__offset = 0; 
    
    if(!result->__ptr){
        PRINT_ERR("malloc failed");
        return SOAP_ERR;
    }

    db = logdb_open(DB_FILE, 15000, 0);

    ppStmt = logdb_etype_list_prep(db);

    while(result->__size < RPC_ETYPE_MAX){
        struct ns__etype  *etype  = result->__ptr + result->__size;
         char *ename, *hname, *unit, *type;
         int eid;
         retval = logdb_etype_list_next(ppStmt, &eid, &ename, &hname, 
                                        &unit, &type);
         
         if(retval != 1)
             break;

         etype->eid = eid;
         etype->name = dg_strdup(ename);
         etype->unit = dg_strdup(unit);
         etype->description = dg_strdup(hname);
	 if(type)
	     etype->type = strdup(type);
	 else
	     etype->type = strdup("");
         result->__size++;
    }
    
    logdb_list_end(ppStmt);  
        
    logdb_close(db);

    return SOAP_OK; 
}

int db_get_events_db(char *begin, char *end, struct ns__events *events, const char *enames)
{
      sqlite3_stmt * ppStmtLog ;
      int retval;
      struct logdb *db;
	  char eids_[1024];
	  char *eids = eids_;

      db = logdb_open(DB_FILE, 15000, 0);

      ppStmtLog  = logdb_evt_list_prep(db, logdb_etype_get_eids(db, enames, eids, sizeof(eids_)), atoi(begin), atoi(end));

      while( events->__size < RPC_EVENT_MAX ){
		  struct ns__event  *event  = events->__ptr + events->__size;       
          int eid;
          char etimestr[32];
          time_t etime = 0;
          const char *eval;
            
          retval = logdb_evt_list_next(ppStmtLog, &eid, &etime, &eval);
          if(retval != 1)
                break;
          sprintf(etimestr, "%ld", etime);

          event->eid = eid;
          event->time = strdup(etimestr);  
          event->value =  strdup(eval);  

          events->__size++;
      }

      logdb_list_end(ppStmtLog);

      logdb_close(db);
     
      return 0;

}


int db_get_events(sqlite3 *db, int row_start, int row_end,char *begin, char *end, char *ename, struct ns__events *events)
{
    sqlite3_stmt *stathandel;
    const char *pzTail;
    int retval;
    int count = row_end - row_start;
    int ptr = 0;

    char query[DB_QUERY_MAX]; 

    PRINT_DBG(0, "count %d", count);

    ptr += snprintf(query+ptr, DB_QUERY_MAX-ptr, 
                   "SELECT time,value FROM event_log WHERE ");
    ptr += snprintf(query+ptr, DB_QUERY_MAX-ptr, 
                    "rowid > %d AND ", row_start);
    if(0x7fffffff != row_end)
        ptr += snprintf(query+ptr, DB_QUERY_MAX-ptr, 
                        "rowid < %d AND ", row_end);

    ptr += snprintf(query+ptr, DB_QUERY_MAX-ptr, 
                    "eid = (SELECT eid FROM event_type WHERE ename='%s')", ename);

    

    retval = sqlite3_prepare_v2(db, query,  strlen(query), &stathandel, &pzTail ); 
     if(retval != SQLITE_OK){
         PRINT_ERR("prepare retval != SQLITE_OK, retval = %d", retval);
         return SOAP_ERR;
    }

    while((retval = sqlite3_step(stathandel)) == SQLITE_ROW){ 
        struct ns__event  *event  = events->__ptr + events->__size++;         

        event->time = strdup(sqlite3_column_text(stathandel, 0));  
        event->value =  strdup(sqlite3_column_text(stathandel, 1));  

        if( events->__size >= RPC_EVENT_MAX ) {
            sqlite3_finalize(stathandel); 
            return SOAP_OK; 
        }
     } 

    if(retval != SQLITE_DONE){
        PRINT_ERR("retval != SQLITE_DONE");
        return SOAP_ERR;
    }
    sqlite3_finalize(stathandel); 
 
    return SOAP_OK; 
    
}


unsigned long timeval_diff(struct timeval *now, struct timeval *prev)
{
    int sec = now->tv_sec - prev->tv_sec;
    int usec = now->tv_usec - prev->tv_usec;

    unsigned long diff;

    if( sec < 0 ) {
        PRINT_ERR("does not support now < prev");
        sec = 0;
    }

    diff = sec*1000;
    diff += usec/1000;

    return diff;
    
        
}


int ns__getEvents(struct soap *soap, char *box, char *ename, char *begin, char *end, struct ns__events_ret *result){

    struct ns__events *events =  &result->events;
    int retval;
/*     sqlite3 *db; */
/*     int row_start, row_end; */
/*     struct timeval tv_start; */
/*     struct timeval tv_cached; */
/*     struct timeval tv_quote; */
    

    result->name = strdup(ename);
    result->begin = strdup(begin);
    result->end = strdup(end);

    events->__ptr = malloc(sizeof(struct ns__event)*(RPC_EVENT_MAX)); 
    events->__size = 0; 
    events->__offset = 0; 
    
    if(!events->__ptr){
        PRINT_ERR("malloc failed");
        return SOAP_ERR;
    }

//    ppStmt = logdb_evt_list_prep(db, -1, lastsend, lastlog);


    db_get_events_db(begin, end, events, ename);
    



/*     retval = sqlite3_open(DB_FILE, &db); */

/*     if(retval){ */
/*         PRINT_ERR("sqlite3_open returned %d", retval); */
/*         sqlite3_close(db); */
/*         return retval; */
/*     } */
    
/*     sqlite3_busy_timeout(db, 60000); */

/*     gettimeofday(&tv_start,NULL); */

/*     db_get_rows(db, atoi(begin),  atoi(end), &row_start, &row_end); */
/*     PRINT_DBG(0, "rows: %d - %d", row_start, row_end); */

/*     gettimeofday(&tv_cached,NULL); */

/*     db_get_events(db,row_start, row_end,begin, end, ename, events); */

/*     gettimeofday(&tv_quote,NULL); */

/*      result->timeCache = timeval_diff(&tv_cached, &tv_start);  */
/*      result->timeQuery = timeval_diff(&tv_quote, &tv_cached);      */
     result->count = events->__size; 
/*     sqlite3_close(db); */
    if( events->__size >= RPC_EVENT_MAX )
        result->complete = "false";
    else
        result->complete = "true";

    return SOAP_OK; 
}
