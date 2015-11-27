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
#include <time.h>
#include <unistd.h>
#include <sdvp.h>
#include <stdbool.h>
#include <syslog.h>
#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <cmddb.h>

#include "contdaem_client.h"
#include "sdvpd_handler.h"
#include "pidfile.h"
#include "version.h"
#include "oom_adjust.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define OOM_ADJUSTMENT 5
#define JIDFILE "/tmp/sdvpd.jid"

#define DEFAULT_PIDFILE "/var/run/sdvpd.pid"
#define DEFAULT_XMLMAP  "/jffs2/mmsmap.xml"

const char *helptext =
    PACKAGE_NAME": Daemon for contact to sdvp.dk XMPP-server\n"
    "Usage: "PACKAGE_NAME" [OPTION]\n"
    " -u, --user=<user>      Set username/jid to <user>\n"
    " -p, --password=<pass>  Set password to <pass>\n"
    " -h, --host=<host>      Set hostname to <host>\n"
    " -m, --map=<mapfile>    Set the MMS mapping file\n"
    " -f, --forground        Run daemon in forground\n"
    " -P, --pidfile          Set pidfile path\n"
    "     --help             Show this help text\n"
    " -v, --version          Show version information\n"
    " -d, --debug=<level>    set debug level to <level>,"
    "                         0: no debug (default)\n"
    "                         1: some debug information\n"
    "                         2: a lot debug information\n";

const char *versiontext = 
    " Version: "AC_VERSION"\n"
    " Build: "BUILD"/"BUILDTAG"\n"
    " By: "COMPILE_BY", "COMPILE_TIME"\n";


static struct option long_options[] =
{
    {"user",        required_argument,        NULL,  'u'},
    {"password",    required_argument,        NULL,  'p'},
    {"host",        required_argument,        NULL,  'h'},
    {"map",         required_argument,        NULL,  'm'},
    {"forground",   no_argument,              NULL,  'f'},
    {"pidfile",     no_argument,              NULL,  'P'},
    {"help",        no_argument,              NULL,  'H'},
    {"version",     no_argument,              NULL,  'v'},
    {"debug",       optional_argument,        NULL,  'd'},
    {NULL, 0, NULL, 0}
};
 
sdvp_config_t* sdvpConfig = NULL;
int run = 0;

void signal_handler (int sig);

void write_username_to_file(const char *username);

int main(int argc, char *argv[])
{

    int c;
    int option_index = 0;	
    int forground = 0;
    const char *hostname = NULL;
    const char *username = NULL;
    const char *password = NULL;
    const char *mmsmap = DEFAULT_XMLMAP;
    const char *pidfile = DEFAULT_PIDFILE;
    int dbglev = 0;

	oom_adjust(OOM_ADJUSTMENT);

#ifdef MEMWATCH
	/* memwatch setup */
    mwStatistics( 2 );
    TRACE("Watching sdvpd!\n");
#endif

    signal(SIGINT, signal_handler);

    while ( (c = getopt_long (argc, argv, 
			      "u:p:h:m:fP:d:vL::H", 
			      long_options, 
			      &option_index)) != EOF ) {    
	switch (c) {
	case 'u':
	    username = optarg;
	    break;
	case 'p':
	    password = optarg;
	    break;
	case 'h':
	    hostname = optarg;
	    break;
	case 'm':
	    mmsmap = optarg;
	    break;
	case 'f':
	    forground = 1;
	    break;
	case 'P':
	    pidfile = optarg;
	    break;
	case 'd':
	    dbglev = atoi(optarg);
	    break;
	case 'v':
	    fprintf(stderr, "%s", versiontext);
	    exit(EXIT_SUCCESS);
	case 'L': // List mms 
	    setlogmask(LOG_UPTO(LOG_DEBUG));
	    sdvpd_handler_InitMap();
	    sdvpd_handler_TestMap(optarg);
	    exit(EXIT_SUCCESS);
	    break;
	case 'H':
	    fprintf(stderr, "%s", versiontext);
	    fprintf(stderr, "%s", helptext);
	    exit(EXIT_SUCCESS);
	default:
	    fprintf(stderr, "%s", helptext);
	    exit(EXIT_FAILURE);
	    break;
	    
	}
    }
    
    openlog(PACKAGE_NAME, LOG_PID|(forground?LOG_PERROR:0), LOG_DAEMON);

    switch(dbglev){
    case 1:
	setlogmask(LOG_UPTO(LOG_INFO));
	break;
    case 2:
	setlogmask(LOG_UPTO(LOG_DEBUG));
	break;
    case 0:
    default:
	setlogmask(LOG_UPTO(LOG_WARNING));
	break;
    }


    if(!hostname||!username||!password){
		fprintf(stderr, "%s", helptext);
	exit(EXIT_FAILURE);
    }

    if(!forground){
	syslog(LOG_NOTICE, "daemonizing\n");
	if(daemon(0, 0)){
	    syslog(LOG_ERR, "daemonize returned error: %s", strerror(errno));
	    exit(1);
	}
    }

    if(pidfile_create(pidfile)!=1){
	syslog(LOG_ERR, "Could not create pidfile\n");
	exit(EXIT_FAILURE);
    }

    /* update pid */
    //    pidfile_pidwrite(pidfile);

    syslog(LOG_DEBUG, "hostname: %s, username: %s, password %s\n",hostname,username,password );


    if(ContdaemClient_Init(NULL)<0){
		syslog(LOG_ERR, "Error opening connection to contdaem\n");
		return EXIT_FAILURE;
    } 

    syslog(LOG_DEBUG, "Contdaem client initiated\n");

    if(cmddb_db_open(NULL, 0)){
		syslog(LOG_ERR, "Error opening cmddb\n");
		return EXIT_FAILURE;
    }

   
    if(sdvpd_handler_InitMapFile(mmsmap)<0){
		syslog(LOG_ERR, "Error loading map\n");
		return EXIT_FAILURE;
    }
    
    sdvpConfig = sdvp_ConfigNew();

    sdvp_ConfigSetUsername(sdvpConfig, username);

	write_username_to_file(username);

    sdvp_ConfigSetPassword(sdvpConfig, password);

    sdvp_ConfigSetHost(sdvpConfig, hostname);

//    sdvp_RegisterCallback(sdvpd_handler_Callback);

    sdvp_RegisterGetNameList(sdvpd_handler_cbGetNameList);

    sdvp_RegisterGetVariableAccessAttribute(sdvpd_handler_cbGetVariableAccessAttributes);
  
    sdvp_RegisterRead(sdvpd_handler_cbRead);
    sdvp_RegisterWrite(sdvpd_handler_cbWrite);
  
  	//Always enable "debug" as syslog is used, the filtering is done later on
    sdvp_ConfigSetDebug(sdvpConfig, SDVP_DEBUG_ENABLED);

    syslog(LOG_DEBUG, "Starting SDVP connection\n");

    sdvp_ConnectionCreate(sdvpConfig);

    syslog(LOG_DEBUG, "Stopped SDVP connection\n");
    sdvp_Shutdown();
    cmddb_db_close();

    ContdaemClient_Deinit(NULL);

    pidfile_close(pidfile);
	
    return 0;
}

void write_username_to_file(const char *username) {
	FILE *fd;
	fd = fopen(JIDFILE, "w");
	fwrite(username, 1, strlen(username)+1, fd);
	fclose(fd);
}

void signal_handler (int sig)
{
    static int killcount = 0;
    
    syslog(LOG_WARNING, "signal: %s (%d)\n", strsignal(sig), sig);
    
    switch(sig) {
    /* case SIGHUP: */
    /* 	if(sdvpConfig) */
    /* 	    sdvp_ConnectionStop(sdvpConfig); */
    /* 	break; */
    case SIGINT:
	if(sdvpConfig)
	    sdvp_ConnectionStop(sdvpConfig);
	run = 0;
	killcount++;
	if(killcount>5) {
	    exit(1);
	}
	break;
    default:
	exit(1);
	break;
    }
    
}

