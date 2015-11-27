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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>

#include "pidfile.h"

int pidfile_pidwrite(const char *pidfile)
{
    FILE *fp;

    if((fp = fopen(pidfile, "w")) == NULL) {
        syslog(LOG_ERR, "Error: could not open pidfile for writing: %s\n", strerror(errno));
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
             syslog(LOG_ERR, "Error: could not open pidfile: %s\n", strerror(errno));
             return -errno;
        }
        
        /* read pid */
        ret = fscanf(fp, "%d[^\n]", &filepid);
        fclose(fp);
        
        if(ret !=1){
            syslog(LOG_ERR, "Error: reading pidfile: %s\n", strerror(errno));
            return -errno;
        }

        syslog(LOG_INFO,"pid in file is  %d\n", filepid);

        if (kill (filepid, 0) == 0){
             syslog(LOG_INFO,"pid is running... \n");
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
