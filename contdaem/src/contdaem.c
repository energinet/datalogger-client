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

#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <sys/stat.h> //pidfile
#include <sys/types.h> //kill
#include <signal.h>  //kill

#include "type_loader.h"
#include "module_type.h"
#include "module_base.h"
#include "module_tick_master.h"

#ifndef PLUGINDIR
/**
 * Default plugin directory */
#define PLUGINDIR "/usr/share/control-daemon/plugins"
#endif
/**
 * Default pid file path */
#define DEFAULT_PIDFILE   "/var/run/contdaem.pid"
/**
 * Default configuration file path  */
#define DEFAULT_CONFFILE  "/etc/contdaem.conf"
/**
 * Default tick interval in ms  */
#define DEFAULT_TICKINT (1000) /*ms*/
/**
 * Default second alignment
 * See @p sec_align in @ref module_tick_master_init
 */
#define DEFAULT_TICKSECAL (1) 


/**
 * @page cmd_options Command line options
 * As default the contdaem will start as a daemon in background using the default configuration file, @p /etc/contdaem.conf,
 * defined in @ref DEFAULT_CONFFILE, and the default plugin directory, @p /usr/share/control-daemon/plugins defined in 
 * @ref PLUGINDIR.
 * The availble options are:
 * <ul>
 *  <li><b> -m \<conf>:</b> Set module configuration file.
 *  <li><b> -P \<dir>:</b> Set plugin directory.
 *  <li><b> -p \<dir>:</b> Set extra plugin directory.
 *  <li><b> -l \<list>:</b> Module load list. This can enable the modules which is disabled in the configuration file.
 *  <li><b> -t \<ms>:</b> Set tick interval in milliseconds. Default is 1000ms which is one tick per second. 
 *  <li><b> -d \<dbglev>:</b>Set debug level:
 *  <li><b> -f:</b> Run the application in forground.
 *  <ul>
 *   <li><b> 0 :</b> no debug (default)
 *   <li><b> 1 :</b> Some debug  
 *   <li><b> 2 :</b> More debug  
 *   <li><b> 3 :</b> A lot of debug  
 *  </ul>
 *  <li><b> -h:</b> Show help.
 * </ul>


 * @see HelpText
 */

/**
 * Command line options 
 */
const  char *HelpText =
 "contdaem: Daemon for logging data and controling peripherals ... \n"
 " -m <conf>    : Set module configuration file (default "DEFAULT_CONFFILE")\n"
 " -P <dir>     : Set plugin directory (default "PLUGINDIR")\n"
 " -p <dir>     : Set extra plugin directory\n"
 " -l <list>    : Module load list. This can enable the modules which is disabled in the configuration file\n"
 " -t <ms_sec>  : Tick interval in milliseconds (default 1000ms, one tick per second)\n"
 " -f           : Run the application in forgroun\n"
 " -d <dbglev>  : Set debug level. 0: none (default), 1: some , 2: more ... \n"
 " -h           : Help (this text)\n"
 "Christian Rostgaard, February 2010\n" 
 "";

int pidfile_pidwrite(const char *pidfile)
{
    FILE *fp;

    if((fp = fopen(pidfile, "w")) == NULL) {
        fprintf(stderr, "Error: could not open pidfile for writing: %s\n", strerror(errno));
        return errno;
    }
    
    fprintf(fp, "%ld\n", (long) getpid());
    
    fclose(fp);

    return 0;
}


int pidfile_create(const char *pidfile)
{
    int ret;   
    struct stat s; 
    FILE *fp;
    int filepid;
    
    ret = stat(pidfile, &s);

    if(ret == 0){
        /* file exits: open file */
        if((fp = fopen(pidfile, "r")) == NULL) {
             fprintf(stderr, "Error: could not open pidfile: %s\n", strerror(errno));
             return -errno;
        }
        
        /* read pid */
        ret = fscanf(fp, "%d[^\n]", &filepid);
        fclose(fp);
        
        if(ret !=1){
            fprintf(stderr, "Error: reading pidfile: %s\n", strerror(errno));
            return -errno;
        }

        printf("pid in file is  %d\n", filepid);

        if (kill (filepid, 0) == 0){
             printf("pid is running... \n");
             return 0;
        }
    }

    pidfile_pidwrite(pidfile);

    return 1;

}


int pidfile_close(const char *path)
{
    if (unlink(path) != 0)
        return -1;
    
    return 0;
  
}


int run = 1;
int hupped = 0;

void signal_handler(int sig)
{

    fprintf(stderr, "signal %d\n", sig);

  switch(sig) {
    case SIGHUP:
      hupped = 1;
      break;
    case SIGINT:
      hupped = 1;
    if(run)
	run = 0;
    else
	exit(0);
    break;
    case SIGPIPE:
      break;
    default:
      break;
  }

}


int main(int narg, char *argp[])
{
    int debug_level = 0;
    int optc;
    struct modules modules;
    char *moduledir = PLUGINDIR;
    char *configfile = DEFAULT_CONFFILE;
    int run_forground = 0;

    struct module_tick_master tick_master;

    int tick_interval = 0;

    memset(&modules, 0, sizeof(modules));

    modules.tick_master = &tick_master;

    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGPIPE, signal_handler);
    signal(SIGALRM, signal_handler);


    while ((optc = getopt(narg, argp, "hp:P:m:d:l:t:f")) > 0) {
        switch (optc){ 
            /* Help */
          case 'h':
            fprintf(stderr, "%s", HelpText);
            exit(0);
            break;
          case 'p': /* load extra module path */
            if( module_type_load_all(optarg, &modules.types) < 0 )
                return -1;    
            break;
          case 'P':
            moduledir = strdup(optarg);
            break;
          case 'm': 
            configfile = strdup(optarg);
            break;
	  case 'l':
	    modules.loadlist = load_list_add(modules.loadlist, optarg);
	    break;
	  case 't': //tick interval
	    tick_interval = atoi(optarg);
	    break;
	  case 'f':
	    run_forground = 1;
	    break;
          case 'd':
            debug_level = atoi(optarg);
            modules.verbose = debug_level;
            break;
          default:
            fprintf(stderr, "%s", HelpText);
            exit(0);
            break;
        }
    }


    module_tick_master_init(&tick_master, DEFAULT_TICKINT,DEFAULT_TICKSECAL  );

    if(!run_forground){
	fprintf(stderr, "daemonizing config file: %s\n", configfile);
	if(daemon(1, 0)){
	    PRINT_ERR("daemon returned error: %s", strerror(errno));
	    exit(1);
	}
    }
	

    pidfile_create(DEFAULT_PIDFILE);
 
    /* load modules */
    if( module_type_load_all(moduledir, &modules.types) < 0 ){
        pidfile_close(DEFAULT_PIDFILE);
        return -1;    
    }

    printf("Types loaded:\n");
    module_type_list_print(modules.types);        
    
    /* load config */
    if(!configfile){
        PRINT_ERR("no config file");
        pidfile_close(DEFAULT_PIDFILE);
        return -1;
    }
    printf("load xml----------------------------++--------------\n");
    
    if(parse_xml_file(configfile, base_xml_tags, &modules)<0)
	return 1;

    printf("do loop----------------------------++--------------\n");
    
     if(tick_interval) 
	 module_tick_master_set_interval(&tick_master, tick_interval);
     
//    tick_master->dbglev = 1;
    module_tick_master_loop(&tick_master, &run);


    printf("deleting modules-----------------------------------------\n");

    
    module_base_delete(modules.list);
    module_type_delete(modules.types);

    printf("contdaem ended\n");

    module_tick_master_stop(&tick_master);

    exit(EXIT_SUCCESS);
    
}
