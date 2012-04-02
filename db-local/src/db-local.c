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

#include <sqlite3.h>

#include "db-local.h"
//#include "uni_data.h"

const  char *HelpText =
 "db-local: Utility for the sqlite database... \n"
 " -n <path>    : Database file\n"
 " -q <quote>   : Quote string\n"
 " -t <timeout> : Set timeout in milliseconds\n"
 " -h           : Help (this text)\n"
 "Christian Rostgaard, February 2010\n" 
 "";





int db_callback(void *data, int argc, char **argv, char **azColName)
{
    int i; 
    
    char *time = NULL;
    char *value = NULL;
    

    for(i=0;i<argc;i++){
        PRINT_DBG(1,"%s = %s", azColName[i], argv[i] ? argv[i] : "NULL");
//        printf("%s,", argv[i] ? argv[i] : "NULL");
            
    }
    
    printf("\n");

    free(time);
    free(value);

    return 0;

}


int db_read_event_callback(void *data, int argc, char **argv, char **azColName)
{
    return 0;
}


int db_test(const char *db_name, const char *db_statement, int timeout)
{
    sqlite3 *db;
    //int data = 34;
    char *zErrMsg = NULL;
    int retval = -EFAULT;

    retval = sqlite3_open(db_name, &db);

    if(retval){
        PRINT_ERR("sqlite3_open returned %d", retval);
        sqlite3_close(db);
        return retval;
    }
    
    if(timeout != 0){
        PRINT_DBG(1, "Setting timeout to %d",timeout);
        sqlite3_busy_timeout(db, timeout);
    }
    

    retval = sqlite3_exec(db, db_statement, db_callback, 0, &zErrMsg);
    
    if(retval != SQLITE_OK) {
        PRINT_ERR("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        retval = -EFAULT;
    } else {
        retval = 0;
    }

    sqlite3_close(db);

    return retval;
}




int main(int narg, char *argp[])
{
    int debug_level = 0;
    int optc;
    int retval;
    int timeout = 15000;
    char *db_name = "testdb";
    char *db_statement = NULL;

    while ((optc = getopt(narg, argp, "hd:n:q:t")) > 0) {
        switch (optc){ 
            /* Help */
          case 'h':
            fprintf(stderr, "%s", HelpText);
            exit(0);
            break;
          case 'd':
            debug_level = atoi(optarg);
            break;
          case 'n': 
            db_name = strdup(optarg);
            break;
          case 'q':
            db_statement = strdup(optarg);
            break;
	  case 't':
	    timeout = atoi(optarg);
	    break;
          default:
            fprintf(stderr, "%s", HelpText);
            exit(0);
            break;
        }
    }

    retval = db_test(db_name, db_statement, timeout);
    
    PRINT_DBG(1,"db_test returned %d", retval);

    return 0;
    
    
}
