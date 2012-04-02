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

#include "licon_check.h"
#include "licon_log.h"
#include "licon_util.h"

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

char *strdup(const char *s);


struct licon_check *licon_check_create(const char *cmd, int expect, int err_max, int interval)
{

    struct licon_check *new = NULL;

    if(!cmd)
	return NULL;

    new = malloc(sizeof(*new));

    assert(new);
    
    memset(new, 0, sizeof(*new));

    new->cmd = strdup(cmd);
    new->expect = expect;
    new->err_max = err_max;
    new->interval = interval;

    return new;
}


void licon_check_delete(struct licon_check *check)
{
    if(!check)
	return;
    
    licon_check_delete(check->next);

    free(check->cmd);
    free(check);
}



void licon_check_reset(struct licon_check *check)
{
    if(!check)
	return;

    licon_check_reset(check->next);
    
    check->err_cnt = 0;
    
}


/**
 * @return the value 0 if OK or -1 if FAULT
 */
int licon_check_do(struct licon_check *check)
{
    int status;
    struct timeval time;

    if(!check)
		return 0;

    gettimeofday(&time,NULL);
	
    if(time.tv_sec < (check->last.tv_sec + check->interval))
		return 0;

    memcpy(&check->last, &time, sizeof(time));
	
    status = system(check->cmd); 
    if(status == -1){
        fprintf(stderr, "could not run \"%s\"\n", check->cmd);
		check->err_cnt++;
		licon_log_eventv("check", "" ,"no run", "(f%d)", check->err_cnt);
    } else if(WEXITSTATUS(status)!=check->expect){
		fprintf(stderr, "\"%s\" returned %d (%d<%d)\n", 
				check->cmd, WEXITSTATUS(status), 
				check->err_cnt, check->err_max);
		check->err_cnt++;
		licon_log_eventv("check", "" ,"fail", "(f%d)", check->err_cnt);
    } else {
		check->err_cnt = 0;
    }
	
    if(check->err_cnt > check->err_max){
		fprintf(stderr, "licon_check_do failed\n");
		licon_log_eventv("check", "" ,"fatal fail", "(f%d)", check->err_cnt);
		return -1;
    }
	
	return 0;
}


int licon_check_status(struct licon_check *check, char* buf, int maxlen)
{
    int ret = 0;
    time_t lastcheck ;

    if(!check)
        return snprintf(buf+ret, maxlen-ret, "check               , errors, maxerr, last check \n");

    lastcheck = licon_time_uptime(check->last.tv_sec, 0);

    snprintf(buf+ret, maxlen-ret , "%s                   ", 
             check->cmd);
    ret += 16;
    ret += snprintf(buf+ret, maxlen-ret, "... ,%5d , %5d , %5ld ", check->err_cnt , check->err_max, lastcheck);

    ret += snprintf(buf+ret, maxlen-ret, "\n");

    return ret;
}

    
