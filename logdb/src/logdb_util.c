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
#include <errno.h>
#include <string.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <getopt.h>
#include "logdb.h"

const char *HelpText = "logdbutil: util for liblogdb... \n"
	" -f <path>       : Database file (default "DEFAULT_DB_FILE")\n"
" -l              : List event types\n"
" -d <level>      : Debug level \n"
" -a              : Add event \n"
" -S <event text> : Add system event \n"
" -g <setting>    : Get setting \n"
" -s <setting>    : Set setting \n"
" -i              : Do integrity check\n"
" -m              ; Do maintenance\n"
"  \n"
" -e <eid>        : eid\n"
" -n <ename>      : ename\n"
" -N \"<text>\"     : hname\n"
" -t <posix time> : time\n"
" -v <value>      : value \n"
" \n"
" -x              : Export data \n"
" -d <level>      : Set debug level \n"
" -h              : Help (this text)\n"
"Copyright (C) LIAB ApS - CRA, June 2010\n"
"";

int list_event_log(struct logdb *db) {
	sqlite3_stmt * ppStmt = logdb_evt_list_prep(db, NULL, 0, 0);
	int retval, eid;
	time_t time;
	char *value;

	/* Get next list */
	while ((retval = logdb_evt_list_next(ppStmt, &eid, &time, &value)) == 1) {
		printf("%d  %d  %s\n", eid, time, value);
	}

	/* end list */
	logdb_list_end(ppStmt);

	return 0;
}

int list_event_types(struct logdb *db) {
	sqlite3_stmt * ppStmt = logdb_etype_list_prep(db);
	int retval, eid;
	char *ename, *hname, *unit, *type;
	char linebuf[80];
	int ptr = 0;
	int count;
	int tot_cnt = 0;
	char tmpstr[32];
	char tmpstr1[32];

	snprintf(linebuf + ptr, 80 - ptr, "eid   cnt ename             ");
	ptr = 22;
	ptr += snprintf(linebuf + ptr, 80 - ptr, "hname");
	printf("%s\n", linebuf);

	/* Get next list */
	while ((retval = logdb_etype_list_next(ppStmt, &eid, &ename, &hname, &unit,
			&type)) == 1) {

		time_t lastlog = 0;
		time_t lastsend = 0;
		logdb_evt_interval(db, eid, &lastlog, &lastsend);
		count = logdb_evt_count(db, eid);

		ptr = 0;
		snprintf(linebuf + ptr, 80 - ptr, "%3d%6d %s             ", eid, count,
				ename);
		ptr = 27;
		ptr += snprintf(linebuf + ptr, 80 - ptr, "%10.10d %10.10d %5s %5s %s",
				lastlog, lastsend, unit, type, hname);
		printf("%s\n", linebuf);
		tot_cnt += count;
	}

	/* end list */
	logdb_list_end(ppStmt);

	printf("total %d/%d, updated %s, server updated %s\n",
			logdb_etype_count(db), tot_cnt, logdb_setting_get(db,
					"etype_updated", tmpstr, 32), logdb_setting_get(db,
					"etype_srv_updated", tmpstr1, 32));

	return 0;
}

int logdb_util_export(struct logdb *db, const char *eids)
{
	   sqlite3_stmt * ppStmtLog ;
      int retval;
	  int t_end    = 1319712143;//time(NULL);
	  int t_begin  = 1319625743;//t_end - (24*3600);
	  int count = 0;
      ppStmtLog  = logdb_evt_list_prep(db, eids,t_begin, t_end);

	  fprintf(stderr, "begin %d --> end %d\n", t_begin, t_end);

      while( 1 ){
		  int eid;
          char etimestr[32];
          time_t etime = 0;
          const char *eval;

          retval = logdb_evt_list_next(ppStmtLog, &eid, &etime, &eval);
		  if(retval != 1)
                break;

		  count++;
      }

	  fprintf(stderr, "count %d \n", count);

      logdb_list_end(ppStmtLog);


      return 0;
}

int main(int narg, char *argp[]) {
	int optc;
	int debug_level = 0;
	struct logdb *db = NULL;
	int eid = -1;
	time_t etime;
	char* value = "0";
	char* filepath = DEFAULT_DB_FILE;
	char *ename = NULL;
	char *hname = NULL;
	struct timeval tv;
	char *setting_name = NULL;
	int get_setting = 0;
	int set_setting = 0;
	int add_event = 0;
	int add_etype = 0;
	int integrity_check = 0;
	int print_events = 0;
	int maintenance = 0;
	int get_write_time = 0;
	int get_buffer_time = 0;
	int removeoldetype = 0;
	char* add_system_event = NULL;
	char* remove_etype = NULL;
	gettimeofday(&tv, NULL);
	etime = tv.tv_sec;
	int retval = 0;
	int last_write = 0;
	int do_export = 0;
	char *export_eids = NULL;

	while ((optc = getopt(narg, argp, "f:le:n:N:Tt:v:ag:s:d:imS:R:w::b::rx::h"))
			> 0) {
		switch (optc) {
		case 'f': // set file path
			filepath = strdup(optarg);
			break;
		case 'l': // list db events
			print_events = 1;
			break;
		case 'e': // eid
			eid = atoi(optarg);
			break;
		case 'T': // Add event type
			add_etype = 1;
			break;
		case 'n': // ename
			ename = strdup(optarg);
			break;
		case 'N': // hname
			hname = strdup(optarg);
			break;
		case 't': // time
			etime = atoi(optarg);
			break;
		case 'v': // value
			value = strdup(optarg);
			break;
		case 'a': // Add event
			add_event = 1;
			break;
		case 'g': // Get setting
			get_setting = 1;
			setting_name = strdup(optarg);
			break;
		case 's': // Set setting
			set_setting = 1;
			setting_name = strdup(optarg);
			break;
		case 'd': // debug level
			debug_level = atoi(optarg);
			break;
		case 'i': // integrity_check
			integrity_check = 1;
			break;
		case 'm':// Maintenance
			maintenance = 1;
			break;
		case 'w': //get last write time
			if (optarg)
				get_write_time = atoi(optarg);
			else
				get_write_time = 3600;
			break;
		case 'b': //get seconds of buffered data
			if (optarg)
				get_buffer_time = atoi(optarg);
			else
				get_buffer_time = 3600 * 24;
			break;
		case 'S':
			add_system_event = strdup(optarg);
			break;
		case 'R': // remove event type
			remove_etype = strdup(optarg);
			break;
		case 'r':
			removeoldetype = 1;
			break;
		  case 'x': //Export data
			do_export = 1;
			if(optarg)
				export_eids = strdup(optarg);
			break;
		case 'h': // Help
		default:
			fprintf(stderr, "%s", HelpText);
			exit(0);
			break;
		}
	}

	if (filepath)
		db = logdb_open(filepath, 15000, debug_level);

	logdb_lock(db);

	if (!db) {
		fprintf(stderr, "ERR: No DB loaded\n");
		return 0;
	}

	if (integrity_check) {
		retval = logdb_check_integrity(db);
		if (retval != 0) {
			fprintf(stderr, "ERR: integrity check failed!\n");
			goto out;
		}

	}

	if(do_export){
		char eids[1024];
		logdb_util_export(db,logdb_etype_get_eids(db, export_eids, eids, sizeof(eids) ));
	}

	if (get_write_time || get_buffer_time) {
		retval = logdb_get_first_int(db, "select MAX(lastlog) from event_type",
				0, &last_write);
		if (retval)
			exit(2);
		printf("last write: %d\n", last_write);
		if ((last_write + get_write_time < time(NULL)) && get_write_time) {
			retval = 1;
		}
	}

	if (get_buffer_time) {
		int last_send = 0;
		retval = logdb_get_first_int(db,
				"select MAX(lastsend) from event_type", 0, &last_send);
		if (retval)
			exit(2);
		printf("last backup: %d\n", last_write - last_send);
		if (last_write - last_send > get_buffer_time) {
			retval = 1;
		}
	}

	if (remove_etype) {
		int eid_ = logdb_etype_get_eid(db, remove_etype);
		logdb_etype_rem(db, eid_);
	}

	if (maintenance)
		logdb_evt_maintain(db);

	if (removeoldetype)
		logdb_etype_removeobs(db);

	if (add_system_event) {
		int sys_eid;

		logdb_etype_add(db, "system", "System Event", "", "hide");
		sys_eid = logdb_etype_get_eid(db, "system");

		logdb_evt_add(db, sys_eid, etime, add_system_event);
	}

	if (add_etype)
		logdb_etype_add(db, ename, hname, NULL, NULL);

	if (add_event)
		logdb_evt_add(db, eid, etime, value);

	if (print_events) {
		list_event_types(db);
	}

	if (get_setting) {
		const char get_value[32];
		printf("Setting: %s  = %s\n", setting_name, logdb_setting_get(db,
				setting_name, get_value, 32));
	}

	if (set_setting) {
		logdb_setting_set(db, setting_name, value);
	}

	out:

	logdb_unlock(db);

	if (maintenance)
		logdb_evt_vacuum(db);

	if (db)
		logdb_close(db);
	
	


	return retval;

}
