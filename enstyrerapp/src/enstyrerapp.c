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
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <logdb.h>
#include <cmddb.h>
#include <contlive.h>
#include <endata.h>
#include <aeeprom.h>

#include "enstyrerapp.h"
#include "enstyrerapp_test.h"
#include "encontrol_simp.h"
#include "timevar.h"


const  char *HelpText =
 "enstyrerapp: Demo showing logdb and cmddb interface... \n"
 " -d<level>      : Set debug level \n"
 " -h              : Help (this text )\n"
 "CRA, Dec 2011\n" 
 "Version: 1.0\n"
 "";



#define AEEVAR_LOCALID "localid"
int enstyrerapp_get_inst_id(int dbglev)
{
	PRINT_DBG(dbglev>=2,"Getting localid\n");
	char *localid = aeeprom_get_entry(AEEVAR_LOCALID); 
	int inst_id = -1;

	if(!localid){
		PRINT_ERR("Error no localid\n");
		return inst_id;
	}

	PRINT_DBG(dbglev, "localid %s\n",localid );
	
	sscanf(localid, "en-%d", &inst_id);
	
	free(localid);

	return inst_id;
	
}

int cmdvar_cmd_handler(CMD_HND_PARAM)
{
    struct timevar *var = (struct timevar*) userdata;
      
	timevar_set(var, cmd->value, cmd->ttime);
   	
    return CMDSTA_EXECUTED;
    
}


/**
 * Init default value */
struct cmddb_desc *cmdvar_initdef(const char *name, const char *defval, struct timevar *var)
{
    char buf[512];
    time_t now = time(NULL);
    time_t t_update = cmddb_value(name, buf , sizeof(buf), now);
	
	if(!t_update){
		cmddb_insert(CIDAUTO, name, defval, now, 0, now);
		timevar_init(var, defval);
	} else {
		timevar_set(var, buf,t_update);
	}
    
	return cmddb_desc_create(name, cmdvar_cmd_handler, var, CMDFLA_SETTING);

}


void enstyrapp_wait_rand(int inst_id, int uptosec)
{
	unsigned short  randnum;
	
	int fd = open("/dev/urandom", O_RDONLY);

	read(fd, &randnum, sizeof(randnum));

	close(fd);

	randnum = randnum/(0xffff/uptosec);

	printf("Waiting %u\n", randnum);

	sleep(randnum);
	

}

int main(int narg, char *argp[])
{
    int optc;

    struct styrerapp styrerobj;
    memset(&styrerobj, 0, sizeof(styrerobj)); 

	char *get_livedata  = NULL;
	enum endata_type get_endata = ENDATA_NONE;
	int do_waitsec = 0;

    while ((optc = getopt(narg, argp, "w::i:l::e::s::d::h")) > 0) {
		switch (optc){ 
		  case 'w':
			if(optarg)
				do_waitsec = atoi(optarg);
			else 
				do_waitsec = 60*10;
			break;
		  case 'c':
			styrerobj.cmdname_control = strdup(optarg);
			break;
		  case 'i':
			styrerobj.inst_id = atoi(optarg);
			break;
		  case 'l':
			if(optarg)
				get_livedata =strdup(optarg);
			else
				get_livedata = "*.*";
			break;
		  case 'e': // test endata interface 
			if(optarg)
				get_endata = atoi(optarg);
			else
				get_endata = ENDATA_ALL;
			break;
		  case 's': // Run schedule planner
			if(optarg){
				styrerobj.schedule_type = strdup(optarg);
			} else {
				styrerobj.schedule_type = strdup(DEFAULT_SCH_PLANNER);
			}
			break;
		  case 'd':
			if(optarg)
				styrerobj.dbglev = atoi(optarg);
			else
				styrerobj.dbglev = 1;
			break;
		  default: /* also h */
			fprintf(stderr, "%s", HelpText);
			exit(0);
			break;
		}
    }

	/* init defaults */

	if(!styrerobj.cmdname_modesch)
		styrerobj.cmdname_modesch = strdup(DEFAULT_CMD_MDSCH);

	if(!styrerobj.cmdname_control){
		styrerobj.cmdname_control = strdup(DEFAULT_CMD_CNTL);
	}

	if(!styrerobj.inst_id){
		styrerobj.inst_id = enstyrerapp_get_inst_id(styrerobj.dbglev);
	}

	/* Do wait to balance load */
	if(do_waitsec)
		enstyrapp_wait_rand(styrerobj.inst_id, do_waitsec);

	/* run test functions if requested */
	if(get_endata != ENDATA_NONE){
		return enapptest_endata(styrerobj.inst_id, styrerobj.dbglev);
	}
	
	if(get_livedata)
		return enapptest_contlive(get_livedata, styrerobj.dbglev);
	
	/* init logdb */
    if((styrerobj.logdb = logdb_open(NULL,  DEFAULT_TIMEOUT, styrerobj.dbglev))==NULL){
		PRINT_ERR("Error opening logdb\n");
		return -1;
    }
	
    PRINT_DBG(styrerobj.dbglev, "logdb opened\n");
    
	/* init commands in cmddb */
	if(cmddb_db_open(NULL, styrerobj.dbglev)<0){
		fprintf(stderr, "error opening cmddb\n");
		return -1;
    }

    PRINT_DBG(styrerobj.dbglev, "cmddb opened\n");
	
    styrerobj.descs = cmddb_desc_add(styrerobj.descs, 
									 cmdvar_initdef(styrerobj.cmdname_modesch, DEFAULT_SCH_PLANNER, &styrerobj.cmdtype));
    
	PRINT_DBG(styrerobj.dbglev, "cmddb descriptor created: %p\n", styrerobj.descs);

	cmddb_exec_list(styrerobj.descs, &styrerobj, time(NULL));

	if(!styrerobj.schedule_type)
		styrerobj.schedule_type = timevar_readcp(&styrerobj.cmdtype);

	PRINT_DBG(styrerobj.dbglev, "schedule mode '%s'\n", styrerobj.schedule_type);

	/* run scheduling */
	encontrol_simp_calc(styrerobj.inst_id      , styrerobj.logdb, 
						styrerobj.schedule_type, styrerobj.dbglev );
	
	/* deinit */
    cmddb_desc_delete(styrerobj.descs);
    logdb_close(styrerobj.logdb);
    cmddb_db_close();

	free(styrerobj.cmdname_modesch);
	free(styrerobj.schedule_type);
	free(styrerobj.cmdname_control);
	
    return 0;
}

