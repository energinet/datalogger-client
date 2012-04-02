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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <assert.h>
#include  <sqlite3.h>
#include <logdb.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>

#define LOGFUNC
#ifdef LOGFUNC
#define LOG(prio, ...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(prio, ...) syslog(prio, __VA_ARGS__)
#endif


#ifdef DBDIR
#define DB_FILE DBDIR "/bigdb.sql"
#else
#define DB_FILE "/jffs2/bigdb.sql"
#endif


#define RPC_ETYPE_MAX 100
#define RPC_EVENT_MAX 30000

#define DELM ";"

struct query_item{
    const char *name;
    const char *value;
};


typedef struct {
    struct logdb *db;
    int drivercount;
    int iid;
} sDSWebservice;




struct eitem {
    char *hname;
    char *ename;
    char *unit;
    int eid;
    float value;    
    time_t time;
    time_t prev;
    int etf;
};



struct query_item* acgi_prep(const char *envdata)
{
    struct query_item *items = malloc(sizeof(struct query_item)*100);
    char *ptr = NULL;
    int i = 0;

    if(!envdata)
	return NULL;
    memset(items, 0 , sizeof(struct query_item)*100);

    ptr = strdup(envdata);

    while(ptr && i < 90){
	struct query_item* item = items + i;
	char *equal = strchr(ptr, '=');
	char *next = strchr(ptr, '&');
	item->name = ptr;

//	printf("item: %s <br>\n",  ptr);

	if(equal && (!next || (equal < next))){
	    item->value = equal+1;
	    equal[0] = '\0';
	} 
	    
	
	if(next){
	    ptr = next+1;
	    next[0] = '\0';
	} else {
	    ptr = NULL;
	}


	i++;
    }

    return items;
    
}

void acgi_arg_print(struct query_item* items)
{
    struct query_item* item = items;

    while(item->name){
	fprintf(stderr, "item: %s , %s;", item->name, item->value);
	item++;
    }
    
    
}





struct query_item* acgi_arg_get(struct query_item* items, const char* name)
{
    struct query_item* item = items;

    while(item->name){
	if(strcmp(item->name, name)==0)
	    return item;
	item++;
    }
    
    return NULL;
    
}


const char* acgi_arg_get_str(struct query_item* items, const char* name)
{
    struct query_item* item = acgi_arg_get(items, name);

    if(item)
	return item->value;
    
    return NULL;
    
}





int acgi_arg_get_int(struct query_item* items, const char* name, int def)
{
    const char* value = acgi_arg_get_str(items, name);

    if(value)
	return atoi(value);
    else
	return def;
}

time_t acgi_arg_get_time(struct query_item* items, const char* name, time_t def)
{
     const char* value = acgi_arg_get_str(items, name);

    if(value)
	return atoi(value);
    else
	return def;
     

}



int etf_get(char **unit_)
{
    char *unit = *unit_;

    if(strcmp(unit, "_J")==0){
	free(*unit_);
	*unit_ = strdup("W");
	return 1;
    }else if(strcmp(unit, "cJ")==0){
	free(*unit_);
	*unit_ = strdup("W");
	return 1;
    }else if(strcmp(unit, "_m³")==0){
	free(*unit_);
	*unit_ = strdup("m³/h");
	return 3600;
    } else
	return 0;
}

char *dg_strdup(const char *db_value)
{
    return strdup(db_value ? db_value : "NULL");
}


int connect_db(sDSWebservice *ds)
{

    
    ds->db = logdb_open(DB_FILE, 15000, 0);

    if(!ds->db)
	return -EFAULT;
    
    LOG(LOG_INFO, "%s():%d - Database "DB_FILE" open\n", __FUNCTION__, __LINE__);

    return 0;
}

void disconnect_db(sDSWebservice *ds)
{
    logdb_close(ds->db);

    return;

}




struct eitem *dataexport_get_enames(sDSWebservice *ds, struct query_item* qitems)
{
    sqlite3_stmt *stathandel;
    const char *pzTail;
    struct logdb *db = ds->db;
    struct eitem *eitems = malloc(sizeof(struct eitem)*RPC_ETYPE_MAX); 
    struct eitem *eitem = eitems; 
    struct query_item* qptr = qitems;
    int maxlen = 2048;
    char query[maxlen]; 
    int qlen = 0;
    int eidcount = 0;
    int retval = 0;

    assert(eitems);

    qlen += snprintf(query+qlen,maxlen-qlen, "SELECT eid,ename,hname,unit FROM event_type WHERE ");

    while((qptr = acgi_arg_get(qptr, "eid"))){
	if(eidcount==0)
	    qlen += snprintf(query+qlen,maxlen-qlen, "eid=%s ",qptr->value );
	else
	    qlen += snprintf(query+qlen,maxlen-qlen, "OR eid=%s ", qptr->value );
	eidcount++;
	qptr++;
    } 
    
    if(!eidcount){
	qlen += snprintf(query+qlen,maxlen-qlen, "type IS NULL");
    }

    retval = sqlite3_prepare_v2(db->db, query,  qlen , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
	fprintf(stderr, "SQL error in prepra of %s\n", query);
	free(eitems);
        return NULL;
    }

    while((retval = sqlite3_step(stathandel))==SQLITE_ROW){
	eitem->eid = sqlite3_column_int(stathandel, 0);
	eitem->ename = dg_strdup((char *)sqlite3_column_text(stathandel, 1));
	eitem->hname = dg_strdup((char *)sqlite3_column_text(stathandel, 2));
	eitem->unit = dg_strdup((char *)sqlite3_column_text(stathandel, 3));
	eitem->etf = etf_get(&eitem->unit);
	eitem->value =  strtod("NAN", NULL); 
	eitem++;
    }

    eitem->ename = NULL;


    switch(retval){
      case SQLITE_DONE:
        break;
      case SQLITE_BUSY:
      case SQLITE_ERROR:
      case SQLITE_MISUSE:
      default:
	free(eitems);
        return NULL;
    }

    sqlite3_finalize(stathandel);

    return eitems;

}

int db_prep_time(char *strtime, int len, time_t time)
{
    struct tm gmt;   
    
    if(!gmtime_r(&time, &gmt)){
        fprintf(stderr, "Datetime error\n");
        return -1;
    }

     return sprintf(strtime, "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", 
            gmt.tm_year+1900, gmt.tm_mon+1, gmt.tm_mday,
            gmt.tm_hour, gmt.tm_min,  gmt.tm_sec);
    
     

}



int dataexport_print_values(struct eitem *eitems, time_t time)
{
    struct eitem *eitem = NULL;
    char timestr[32];
    
    db_prep_time(timestr, 32, time);

    printf("%s", timestr);

    eitem = eitems;
    while(eitem->ename){
	printf(",%f", eitem->value);
	eitem++;
    }

    printf("\n");

    return 0;

}



int dataexport_set_value(struct eitem *eitems, int eid, float value, time_t time)
{
    struct eitem *eitem = NULL;

    eitem = eitems;
    while(eitem->ename){
	if(eitem->eid == eid){
	    if(eitem->etf==0){
		eitem->value = value;
		eitem->time  = time;
	    } else {
		time_t diff = time - eitem->time;
		if(time){
		    eitem->value = (value / diff)*eitem->etf;
		}
		if(value)
		    eitem->time  = time;
		
	    }

	    return 1;
	}
	eitem++;
    }

    return 0;

}




int dataexport_get_events(sDSWebservice *ds, struct eitem *eitems, time_t start, time_t end)
{

    time_t prev= 0;
    
    sqlite3_stmt *stathandel;
    const char *pzTail;
    int maxlen = 2048;
    char query[maxlen]; 
    int qlen = 0;
    int retval;
    struct logdb *db = ds->db;

    qlen += snprintf(query+qlen,maxlen-qlen, 
		     "SELECT eid, time, value FROM event_log" 
		     " WHERE time > %lu AND time < %lu" 
		     " ORDER BY time ASC ",  start, end); 

    
    retval = sqlite3_prepare_v2(db->db, query,  qlen , &stathandel, &pzTail );
    

    
    if(retval!=SQLITE_OK){
	fprintf(stderr, "SQL error in prepra of %s\n", query);
        return -EFAULT;
    }


    while((retval = sqlite3_step(stathandel))==SQLITE_ROW){

	time_t time= sqlite3_column_int(stathandel, 1);
	int eid = sqlite3_column_int(stathandel, 0);
	float value = sqlite3_column_double(stathandel, 2);

	if(prev==0) 
	    prev = time; 
	else if(prev != time){ 
 	    if(!(prev%300)) 
		dataexport_print_values(eitems, prev); 
	    prev = time;	 
	} 

	dataexport_set_value(eitems, eid, value, time); 
    } 
    
    switch(retval){
      case SQLITE_DONE:
        break;
      case SQLITE_BUSY:
      case SQLITE_ERROR:
      case SQLITE_MISUSE:
      default:
	return -EFAULT;
    }

    return 0;

}



char* ecstrdup(char *str, char end)
{
    char *ptr = NULL;
    char *ret = NULL;
    int size = 0;

    if(!str)
	return NULL;
    
    ptr = strchr(str, end);
    
    if(!ptr)
	return strdup(str);

    size = (ptr-str);
    ret = malloc(size+1);

    memcpy(ret, str, size);
    ret[size] = '\0';

    return ret;

}


int dataexport_print_header(struct eitem *eitems)
{
    struct eitem *eitem = NULL;

    assert(eitems);
    printf("Kortnavn");

    eitem = eitems;
    while(eitem->ename){
	printf(DELM"%s", eitem->ename);
	eitem++;
    }

    printf("\n");
    printf("Navn");

    eitem = eitems;
    while(eitem->ename){
	printf(DELM"\"%s\"", eitem->hname);
	eitem++;
    }
    printf("\n");
    printf("Enhed");
    eitem = eitems;
    while(eitem->ename){
	printf(DELM"[%s]", eitem->unit);
	eitem++;
    }
    printf("\n");

    return 0;

}



int main(void)
{
    struct query_item* items;
    struct eitem *eitems;
    char *data;
    sDSWebservice ds;
    time_t t_start;
    time_t t_end;
    struct timeval tv;
    char ts_start[32];
    char ts_end[32];

    gettimeofday(&tv, NULL);

    data = getenv("QUERY_STRING");
    items = acgi_prep(data);
    connect_db(&ds);
    
    eitems = dataexport_get_enames(&ds, items);
    
    printf("%s%c%c\n",
	   "Content-Type:text/csv;charset=utf-8",13,10);

    t_start = acgi_arg_get_time(items, "timeStart" ,tv.tv_sec-(24*60*60));

    t_end   = acgi_arg_get_time(items, "timeEnd" ,tv.tv_sec);


    db_prep_time(ts_start, sizeof(ts_start), t_start);

    db_prep_time(ts_end, sizeof(ts_end), t_end);

    dataexport_print_header(eitems);

    dataexport_get_events(&ds, eitems, t_start, t_end);

    disconnect_db(&ds);


    return 0;
}
//csv_export.cgi
