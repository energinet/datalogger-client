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

#include "licon_log.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <stdarg.h>
#include <pthread.h>
#include <assert.h>

struct licon_log *licon_log_list_rem_(struct licon_log *first, struct licon_log *rem);

#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s: "str"\n",__FUNCTION__, ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}


int sysdbg = 0;
#define LOGMASK (LOG_MASK(LOG_ERR) | LOG_MASK(LOG_NOTICE) | LOG_MASK(LOG_INFO))
#define LOGMASKDBG (LOG_MASK(LOG_ERR) | LOG_MASK(LOG_NOTICE) | LOG_MASK(LOG_INFO) |  LOG_MASK(LOG_DEBUG))


void licon_sysdbg_set(int set)
{
    sysdbg = set;

    if(set){
	 openlog("licon", LOG_PID, LOG_DAEMON);  
	 if(set==2)
	     setlogmask(LOGMASKDBG);
	 else
	     setlogmask(LOGMASK);
    } 

}

void licon_sysdbg_log(int priority, const char *format,...)
{
    va_list ap;
    va_start(ap, format);

    if(sysdbg)
	vsyslog(priority, format, ap);
    else
	vfprintf(stderr,format, ap);
    va_end(ap);

}


struct licon_log *loglist = NULL;
pthread_mutex_t loglist_mutex = PTHREAD_MUTEX_INITIALIZER;

void licon_log_print_(struct licon_log *log)
{
	if(!log)
		return;

	if(log->next)
        licon_log_print_(log->next);

    printf("%s   %s\n", ctime(&log->tv.tv_sec), log->text);
}


void licon_log_print()
{
	int retval;
	
	if((retval = pthread_mutex_lock(&loglist_mutex))!=0){
		fprintf(stderr, "Error: Log mutex: %s (%d)\n", strerror(-retval), retval);
		return;
	}

	licon_log_print_(loglist);

	pthread_mutex_unlock(&loglist_mutex); 
           
}



struct licon_log *licon_log_create(const char *type, const char *name, const char *event )
{

	return licon_log_createv(type, name, event, NULL);

}

struct licon_log *licon_log_createv(const char *type, const char *name, const char *event, const char *format, ...)
{
    struct licon_log *new = malloc(sizeof(*new));
    char text[512];
	int len = 0;

    assert(new);
    memset(new, 0 , sizeof(*new));

    gettimeofday(&new->tv, NULL);

    len = snprintf( text+len , sizeof(text)-len , "%s:%s %s", type, name, event);

	if(format){
		va_list ap;		
		va_start(ap, format);
		len += vsnprintf(text+len, sizeof(text)-len, format, ap);
		va_end(ap);
	}

    new->text = strdup(text);

    PRINT_LOG(LOG_INFO, "logevent: %s\n",new->text);

    return new;
    
}


void licon_log_delete(struct licon_log *log)
{
    if(log->next)
        licon_log_delete(log->next);

    free(log->text);
    free(log);
}

struct licon_log *licon_log_list_add_(struct licon_log *first, struct licon_log *new)
{
    struct licon_log *ptr = first;
    int count = 0;

    if(!first){
        return new;
    }
	
    while(ptr->next){
        count++;
        ptr = ptr->next;
    }
    ptr->next = new;

    if(count>5){
        /* remove the oldest log */
        struct licon_log *rem = first; 
        first = licon_log_list_rem_(first, rem);
        licon_log_delete(rem);
    }

    return first;

}

void licon_log_list_add(struct licon_log *new)
{
	int retval;

	if((retval = pthread_mutex_lock(&loglist_mutex))!=0){
		fprintf(stderr, "Error: Log mutex: %s (%d)\n", strerror(-retval), retval);
		return;
	}

    loglist = licon_log_list_add_(loglist, new);

	printf("Log list:\n");
	licon_log_print_(loglist);

	pthread_mutex_unlock(&loglist_mutex); 
}

struct licon_log *licon_log_list_rem_(struct licon_log *first, struct licon_log *rem)
{
    struct licon_log *ptr = first;
    struct licon_log *next = NULL;
    
	if(!rem)
		return next;

    if(ptr == rem){ //First item is to be removed
        next = rem->next;
        rem->next = NULL;
        return next;
    }

    while(ptr){
        if(ptr->next == rem){
            ptr->next = rem->next;
            rem->next = NULL;
        }
    }
 
    return first;
}



void licon_log_event(const char *type, const char *name, const char *event)
{

	licon_log_list_add(licon_log_create(type, name, event));

}





void licon_log_eventv(const char *type, const char *name, const char *event, const char *format, ...)
{
	char text[512];
	int len = 0;
	va_list ap;		

	len = snprintf( text+len , sizeof(text)-len , "%s ", event);

	va_start(ap, format);
	len += vsnprintf(text+len, sizeof(text)-len, format, ap);
	va_end(ap);
	
	licon_log_list_add(licon_log_create(type, name, text));

}


/* struct licon_log *licon_log_last(void) */
/* { */
/*     struct licon_log *ptr = loglist; */
    
/*     if(!ptr) */
/*         return NULL; */

/*     while(ptr->next) */
/*         ptr = ptr->next; */

/*     return ptr; */
/* } */


struct licon_log *licon_log_rgetfirst(void)
{
	int retval = 0;
	struct licon_log *ptr = NULL;
	
	if((retval = pthread_mutex_lock(&loglist_mutex))!=0){
		fprintf(stderr, "Error: Log mutex: %s (%d)\n", strerror(-retval), retval);
		return NULL;
	}
	
	ptr = loglist;

	
	loglist = licon_log_list_rem_(loglist, ptr);

	pthread_mutex_unlock(&loglist_mutex); 

    return  ptr;
}

/* void licon_log_rem(struct licon_log *log) */
/* { */
/* 	int retval; */

/* 	if((retval = pthread_mutex_lock(&loglist_mutex))=!0){ */
/* 		fprintf(stderr, "Error: Log mutex: %s (%d)\n", stderror(-retval), retval); */
/* 		return; */
/* 	} */


/*     loglist = licon_log_list_rem(loglist, log); */
/*     licon_log_delete(log); */


/* } */

/* int licon_log_send(char* buf, int maxlen) */
/* { */
/*     int len = 0; */

/*     struct licon_log *log = licon_log_last(); */

/*     if(!log) */
/*         return 0; */

/*     len = snprintf(buf-len, maxlen-len, "%ld %s", log->tv.tv_sec, log->text); */

/*     loglist = licon_log_list_rem(loglist, log); */

/*     licon_log_delete(log); */

/*     return len; */
/* } */


