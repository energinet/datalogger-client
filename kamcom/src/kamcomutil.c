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
#include "kamcom.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <syslog.h>
#include "version.h"

#define DEFAULT_PATH "/dev/ttyS1"
#define DEFAULT_BAUD 1200

const  char *helptext =
"kamcomutil: Utility program for the Kamstrup communication library. Used for testing and debugging\n"
" -p , --path=<path>        Path to the serial port\n"
" -b , --baud=<baud>        Serial baud rate\n"
" -g , --get=<register>     Get register\n"
" -s , --scan               Scan addresses\n"
" -A , --acrivate           Activate communication\n"
" -h , --help               Show this help text\n"
" -v , --verbose            Show verbose information\n"
    //" Version: "AC_VERSION"\n"
    //" Build: "BUILD"/"BUILDTAG"\n"
" By: "COMPILE_BY", "COMPILE_TIME"\n";


static struct option long_options[] =
{
    {"path",    required_argument,  NULL, 'p'},
    {"baud",    required_argument,  NULL, 'b'},
    {"get",     required_argument,  NULL, 'g'},
    {"activate",no_argument,        NULL, 'A'},
    {"scan",    no_argument,       NULL, 's'},
    {"help",    no_argument,        NULL, 'h'},
    {"verbose", no_argument,        NULL, 'v'},
    {NULL, 0, NULL, 0}
};
 
int argatoi(const char *str, int def)
{
    int value;
    if(!str)
	return def;

    if(sscanf(str, "0x%x", &value)==1)
	return value;

    if(sscanf(str, "%d", &value)==1)
	return value;

    fprintf(stderr, "ERROR reading string %s\n", str);

    return def;
}


/*
 * Main 
 */

int main(int argc, char *argv[]) 
{
    int c;
    int option_index = 0;	
    int verbose = 0;
    const char *path = DEFAULT_PATH;
    int baud_tx = DEFAULT_BAUD;
    int baud_rx = DEFAULT_BAUD;
    int activate = 0;
    int addr = 0x3f;
    int getreg = 0;
    int scan = 0;

    while ( (c = getopt_long (argc, argv, 
			      "p:b:t:r:a:g:sAhv", 
			      long_options, 
			      &option_index)) != EOF ) {    
	switch (c) {
	case 'p':
	    path = optarg;
	    break;
	case 'b':
	    baud_tx = atoi(optarg);
	    baud_rx = atoi(optarg);
	    break;
	case 't':
	    baud_tx = atoi(optarg);
	    break;
	case 'r':
	    baud_rx = atoi(optarg);
	    break;
	case 'a':
	    addr = atoi(optarg);
	    break;
	case 'g':
	    getreg = argatoi(optarg, 0);
	    break;
	case 's':
	    scan = 1;
	    break;
	case 'A':
	    activate =  1;
	    break;
	case 'v':
	    verbose++;	
	    break;
	case 'h':
	    fprintf(stderr, "%s", helptext);
	    exit(EXIT_SUCCESS);
	default:
	    fprintf(stderr, "%s", helptext);
	    exit(EXIT_FAILURE);
	    break;
	}
    }

    openlog("kamcomutil", LOG_PID|LOG_PERROR, LOG_USER);

    switch(verbose){
    case 0:
	setlogmask(LOG_UPTO(LOG_WARNING));
	break;
    case 1:
	setlogmask(LOG_UPTO(LOG_NOTICE));
	break;
    case 2:
	setlogmask(LOG_UPTO(LOG_INFO));
	break;
    default:
    case 3:
	setlogmask(LOG_UPTO(LOG_DEBUG));
	break;
    }
    
    syslog(LOG_INFO, "Logmask is verbose %d (%d/%d)", verbose, baud_tx, baud_rx);
    
    struct kamcom * kam = kamcom_create(path, baud_tx, baud_rx);
    
    if(!kam){
	syslog(LOG_ERR, "Could not create kamcom object");
	exit(EXIT_FAILURE);
    }
    
    if(activate)
	kamcom_activate(kam);
    

    if(getreg){
	struct kamcom_cmd *cmd;
	unsigned char unit;
	fprintf(stdout, "Reading register: %d (0x%x)\n", getreg, getreg);
	cmd = kamcom_cmd_create_getreg(addr, getreg);
	kamcom_trx(kam, cmd);
	float value = 0;
	if(kamcom_cmd_rd_regf(cmd, getreg, &value, &unit)){
	    fprintf(stderr, "Read error\n");
	} else {
	    fprintf(stdout, "Read register: %f %s (0x%x)\n", value, kamcom_types_unit(unit), unit);
	}
	kamcom_cmd_delete(cmd);
    } else if (scan) {
	int i ;
	struct kamcom_cmd *cmd;
	kam->retry = 1;
	for(i = 0; i < 256 ; i++){
	    setlogmask(LOG_UPTO(LOG_EMERG));	    
	    cmd = kamcom_cmd_create(i, 0x01);
	    printf("addr 0x%X; %d\n",i,  kamcom_trx(kam, cmd));
	    kamcom_cmd_delete(cmd);
	}
    } else {
	kamcom_test(kam, addr);
    }
    
    kamcom_delete(kam);
    
    exit(EXIT_SUCCESS);
	
    return 0;
}
