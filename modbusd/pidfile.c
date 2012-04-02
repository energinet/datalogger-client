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

#include "pidfile.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/file.h>

int pidfile_open(struct pidfile *pidfile)
{
    int lock_flags = LOCK_EX;
    char buffer[32];
    int len = 0;
    
    if((pidfile->fd = open(pidfile->path, O_WRONLY|O_CREAT)) < 0) {
        fprintf(stderr, "Error: could not open pidfile for writing: %s\n", strerror(-pidfile->fd));
        return -pidfile->fd;
    }

    if(pidfile->mode != PMODE_KILL)
	lock_flags |= LOCK_NB;


    if(flock(pidfile->fd, lock_flags)){
	fprintf(stderr, "Could not get file lock for %s: %s\n", pidfile->path, strerror(errno));
	close(pidfile->fd);
	return -EALREADY;
    }    
    
    len = sprintf(buffer, "%ld\n", (long) getpid());

    if(write(pidfile->fd, buffer, len)!= len){
	fprintf(stderr, "Error on fsync: %s\n", strerror(errno));
    }

    if(fsync(pidfile->fd)){
	fprintf(stderr, "Error on fsync: %s\n", strerror(errno));
    }

    return 0;
}


void pidfile_close(struct pidfile *pidfile)
{
    
    if(pidfile->fd){
	flock(pidfile->fd, LOCK_UN);
	close(pidfile->fd);    
    }

    if(pidfile->path)
	unlink(pidfile->path);

    return;
}


/**
 * Check if application is running 
 * @return pid if running, 0 if not running or -errno if error
 */

int pidfile_check(const char *pidfile)
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
             return filepid;
        }
    }

    return 0;
    
}


void pidfile_delete(struct pidfile *pidfile)
{
    if(!pidfile)
	return;

    pidfile_close(pidfile);

    free(pidfile->path);
    free(pidfile);
}

struct pidfile* pidfile_create_(const char *path, enum pidfile_mode mode, int dbglev)
{
     struct pidfile* pidfile = malloc(sizeof(*pidfile));
     int retval = 0;
     assert(pidfile);
     memset(pidfile, 0, sizeof(*pidfile));
     
     pidfile->path = strdup(path);
     pidfile->mode = mode;
     pidfile->dbglev = dbglev;

     if((retval = pidfile_open(pidfile))!=0){
	 pidfile_delete(pidfile);
	 return NULL;
     }

     return pidfile;
}



struct pidfile* pidfile_create(const char *path, enum pidfile_mode mode, int dbglev)
{
   
    int filepid;
    
    filepid = pidfile_check(path);
    
    if(filepid == 0){
	return pidfile_create_(path, mode, dbglev);
    } else if (filepid < 0){
	return NULL;
    }

    fprintf(stderr, "Application is allready running (pid: %d)\n", filepid);
        
    switch(mode){
      case PMODE_HUP:
	kill (filepid, SIGHUP); 
	/* continues */
      case PMODE_RETURN:
	return NULL;
      case PMODE_KILL:
	kill(filepid, SIGKILL); 
	return pidfile_create_(path, mode, dbglev);
      case PMODE_EXIT:
	exit(0);
    }

    return NULL;

}

