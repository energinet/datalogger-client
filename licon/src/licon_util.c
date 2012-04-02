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

#include "licon_util.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

int licon_file_read(const char *path, char* buf, size_t size)
{
	FILE *file ;
	int retval = 0;

	if((file = fopen(path,"r"))==NULL) 
        return -1;

	retval = fread(buf, 1, size, file);

	if(retval == 0 && !feof(file)){
		fprintf(stderr, "File error: %d", ferror(file));
		fclose(file);
		return -1;
	} else {
		fprintf(stderr,"file read %d\n", retval);
		buf[retval]= '\0';
	}
	 
	fclose(file);
	
	return retval;
	

}


int licon_pidfile_pid(const char *pidfile)
{
    FILE *file;
    int pid;

    if(!pidfile)
        return -1;

    file = fopen(pidfile, "r");

    if(!file)
        return -errno;

    if(fscanf(file, "%d", &pid)!=1){
        pid =  -EFAULT;
    }
    
    fclose(file);

    return pid;

}



int licon_pidfile_check(const char *pidfile)
{
    FILE *file;
    int pid;
    int retval = 0;
//    printf("Check pidfile: %s\n", pidfile);
    file = fopen(pidfile, "r");

    if(!file)
        return -EFAULT;

    if(fscanf(file, "%d", &pid)!=1){
        retval = -EFAULT;
        goto out;
    }

    if (kill (pid, 0) == -1){
        retval = errno;
        goto out;
    }

  out:
    fclose(file);
    
    return retval;

}

int licon_pidfile_hup(const char *pidfile)
{
    FILE *file;
    int pid;
    int retval = 0;

    file = fopen(pidfile, "r");

    if(!file)
        return -EFAULT;

    if(fscanf(file, "%d", &pid)!=1){
        retval = -EFAULT;
        goto out;
    }

    if (kill (pid, SIGHUP) == -1){
        retval = errno;
        goto out;
    }

  out:
    fclose(file);
    
    return retval;

}




int licon_pidfile_stop(const char *pidfile)
{
    FILE *file;
    int pid;
	int timeout = 60;

    printf("Stopping pidfile: %s\n", pidfile);
    file = fopen(pidfile, "r");

    if(!file)
        return 0;

    printf("Pidfile opened\n");


    if(fscanf(file, "%d", &pid)==1){
        printf("Killing Pid %d\n", pid);
        kill(pid, SIGINT);
	while(kill (pid, 0)==0){
	    sleep(1);
	    if(timeout--<=0)
		kill(pid, SIGKILL);
	}
    }

    fclose(file);

    return 0;
}


int licon_cmd_run(const char *cmd){
    int status;

    if(!cmd)
	return 0;

    status = system(cmd); 
    if(status == -1){
        fprintf(stderr, "Error running \"%s\"\n", cmd);
        return -EFAULT;
    } 
    
    return WEXITSTATUS(status); 

}


char *licon_strdup(const char *str)
{
    if(str)
        return strdup(str);
    else
        return NULL;
}


char *licon_defdup(const char *str, const char *def)
{
    if(str)
        return strdup(str);
    else
        return strdup(def);
}



char *licon_platform_conf(const char *name)
{
    FILE *file;
    char sstring[128];
    char value[128];
    char line[256];
    int linemax = 100;
    sprintf(sstring, "%s=%%s\\n", name);

    file = fopen("/tmp/optionsfile", "r");
    
    if(file == NULL) {
        fprintf(stderr,"%s(): %d: Error opening file '%s'\n", __FUNCTION__, __LINE__, "/tmp/optionsfile");
        return NULL;
    }

    strcpy(value, "N/A");
    
    while(!feof(file)){

        if(!fgets(line, 256, file))
            break;

        if(sscanf(line, sstring, value)==1)
            break;      
        
        
        if(linemax<=0)
            break;

        linemax--;
    }

    fclose(file);

    return strdup(value);


}


time_t licon_time_uptime(time_t tstart, time_t now)
{
    
    if(!tstart)
	return 0;

    if(!now){
	now = time(NULL);
    }
    
    if(tstart > now )
	return 0;

    return now - tstart;

}


time_t licon_time_until(time_t timeout, time_t now)
{

    if(!timeout)
		return 0;

    if(!now){
		now = time(NULL);
    }
	
    if(now > timeout)
		return 0;

    return timeout - now;
    
}


void licon_waittime_print(struct licon_waittime *waittime)
{
    int i;

    if(!waittime){
	fprintf(stderr,"default wait times\n");
	return;
    }
    
    fprintf(stderr,"custom wait times: ");

    for( i = 0 ; i < waittime->size ; i++)
	fprintf(stderr,"%d,", waittime->waittimes[i]);

    fprintf(stderr,"\n");
    
    
}

struct licon_waittime *licon_waittime_create(const char *str)
{
    struct licon_waittime *new;
    const char *ptr = str;
    int waittimes[512];

    if(!str) 
	return NULL;

    new = malloc(sizeof(*new));

    assert(new);

    new->size = 0;

    while((ptr) && (new->size < sizeof(waittimes)/sizeof(int) )){
	waittimes[new->size++] = atoi(ptr);
	ptr=strchr(ptr, ',');

	if(ptr)
	    ptr++;
	
    }

    new->waittimes = malloc(sizeof(int)*new->size);

    memcpy(new->waittimes, waittimes, sizeof(int)*new->size);
    new->waittimes[new->size] = 0;

    licon_waittime_print(new);

    return new;
}

void licon_waittime_delete(struct licon_waittime *waittime)
{
    if(waittime){
        free(waittime->waittimes);
	free(waittime);
    }

}


int licon_time_timout_get(int errcnt, struct licon_waittime *waittime)
{
    
    int waittimes_def[] = { 5, 60 , 300, 300, 300, 600, 3600, 3600, 3600 };
    int size_def  = sizeof(waittimes_def)/sizeof(waittimes_def[0]);
    int *waittimes = waittimes_def;
    int size = size_def;

    if(waittime){
		waittimes = waittime->waittimes;
		size = waittime->size;
    }

    if(errcnt < 0)
		return waittimes[0];

    if(errcnt >= size)
		return waittimes[size-1];

    return waittimes[errcnt];

}

time_t licon_time_get_next_chk(int errcnt, time_t now, struct licon_waittime *waittime)
{
    if(!now){
	now = time(NULL);
    }

    return licon_time_timout_get(errcnt, waittime) + now;

}
