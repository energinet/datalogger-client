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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <syslog.h>
#include <asocket_trx.h>
#include <asocket_client.h>
#include <qDecoder.h>
#include <errno.h>

#include <logdb.h>

#include "siteutil.h"


#define LOGFUNC
#ifdef LOGFUNC
#define LOG(prio, ...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(prio, ...) syslog(prio, __VA_ARGS__)
#endif

/* struct etype_header{ */
/* 	int eid; */
/* 	char *ename; */
/* 	char *hname;  */
/* 	char *unit;  */
/* 	char *type; */
/* 	struct etype_header *next; */
/* }; */


/* char *strdupnull(const char *str) */
/* { */
/* 	if(str){ */
/* 		return strdup(str); */
/* 	} else { */
/* 		return NULL; */
/* 	} */

/* } */

/* struct etype_header * etype_header_create(const int eid, const char *ename, const char *hname, const char *unit, char *type) */
/* { */
/* 	struct etype_header *new = malloc(sizeof(*new)); */
	
	
/* 	strdupnull(const char *str) */
	
/* } */

/* struct etype_header * etype_header_list_add(struct etype_header *list, struct etype_header *new) */
/* { */
	
/* } */

/* void etype_header_delete(struct etype_header *etype) */
/* { */
	
/* } */


int dbfile_eid(struct logdb *db, int eid)
{
	int retval;
	int eid_ ;
	time_t etime;
	const char *value;

	sqlite3_stmt *ppStmt = logdb_evt_list_prep_send(db, eid);



	while((retval = logdb_evt_list_next(ppStmt, &eid_, &etime, &value))<0){
		printf(",%ld:%s", etime, value);
	}
	
	logdb_list_send_end(ppStmt, db, eid, etime);
	
	return 0;
}


int dbfile_data(struct logdb *db)
{
	int retval;
	int eid ;
	time_t etime;
	const char *value;

	sqlite3_stmt *ppStmt = logdb_evt_list_prep(db, NULL, 0,0);
	printf("------data------\n");

	while((retval = logdb_evt_list_next(ppStmt, &eid, &etime, &value))>0){
		printf("%d;%ld;%s\n", eid,etime, value);
	}
	
	logdb_list_end(ppStmt);

//	printf("\n");
	
	return 0;
}



int dbfile_header(struct logdb *db)
{
	char buffer[1024];
	int len;
	int retval;
	
	int eid;
	char *ename;
	char *hname; 
	char *unit; 
	char *type;

	

	syslog(LOG_DEBUG, "Prepping");
	
	sqlite3_stmt *ppStmt = logdb_etype_list_prep(db);

	syslog(LOG_DEBUG, "Exporting headers");

	printf("------header------\n");

	while((retval = logdb_etype_list_next(ppStmt, &eid, &ename,&hname, &unit,  &type))>0){
		len = sprintf(buffer, "%d;%s;%s;%s;%s\n",eid, ename,hname, unit,type);
		syslog(LOG_DEBUG, "Exporting %d %s", eid, ename);
		fwrite(buffer, 1 , len,stdout);

//		dbfile_eid(db, eid);
//		printf("\n");
	}
	
	/* end list */
	syslog(LOG_DEBUG, "Exported headers");
	logdb_etype_list_end(ppStmt);

	return 0;
}

const char *datetime_get(char *buf)
{
	time_t t = time(NULL);
	struct tm *tm = gmtime(&t);
	
	sprintf(buf, "%4.4d-%2.2d-%2.2d_%2.2d:%2.2d:%2.2d",
			tm->tm_year +1900, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);

	return buf;

}


int main(int argc, char *argv[])
{
	struct sitereq site;
	char buf[128];
	struct logdb *db = logdb_open(DEFAULT_DB_FILE, 15, 0);

	siteutil_init_notpe(&site, "DB-file", "dbfile");

//	logdb_lock(db);

    openlog("dbfile.cgi", LOG_PID, LOG_DAEMON|LOG_PERROR);

	setlogmask(LOG_UPTO(LOG_DEBUG));

	syslog(LOG_DEBUG, "Exported");

	printf("%s\n",
		   "Content-Type:application/x-sqlite3");

	printf("Content-disposition: attachment; filename=dbexport_%s.log\n",datetime_get(buf));
	printf("%c%c",13,10);

	printf("------box------\n%s\n", site.boxid);

	dbfile_header(db);

	dbfile_data(db);

//	logdb_unlock(db);
	logdb_close(db);

	siteutil_deinit(&site);
 
				   
    syslog(LOG_DEBUG, "Exported");
    return EXIT_SUCCESS;

}
