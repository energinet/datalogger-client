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

#include "licon_app.h"
#include "licon_util.h"
#include "licon_log.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

struct licon_app *licon_app_create(const char* name,int type, const char *cmd_start, 
                                   const char *cmd_stop, const char *pidfile)
{
    struct licon_app *new = NULL;

    if(!name){
        return NULL;
    }

    printf("app create: %s\n", name);

    new = malloc(sizeof(*new));

    if(!new){
        fprintf(stderr, "Could not allocate memory for licon_app\n");
        return NULL;
    }
    

    new->fail = 0;
    new->failtot = 0;
    new->next = NULL;
    new->status = APPSTA_STOP;
    new->name = strdup(name);
    new->type = type;
    new->cmd_start = licon_strdup(cmd_start);
    new->cmd_stop = licon_strdup(cmd_stop);
    new->pidfile = licon_strdup(pidfile);
	new->errfile = NULL;
    new->chk_next = 0;
    new->uptime_ok = 90;
    new->time_started = 0;
    new->dbglev = 1;
    new->waittime = NULL;
    new->ignorerr = 0;
    printf("created: %s\n", name);
    return new;
}

struct licon_app *licon_app_add(struct licon_app *first, struct licon_app *new)
{
    struct licon_app *ptr = first;
    printf("add app: %s \n", new->name);

    if(!first){
        return new;
    }

    while(ptr->next){
        ptr = ptr->next;
    }
    ptr->next = new;
    printf("first app: %s \n", first->name);
    return first;
}


struct licon_app *licon_app_get(struct licon_app * list, const char* name)
{
    struct licon_app *ptr = list;
    
    while(ptr){
	if(strcmp(ptr->name, name)==0)
	    return ptr;
	ptr = ptr->next;
    }
    
    return NULL;

}



const char *licon_app_status_text(int status)
{
    switch(status){
      case APPSTA_FATAL:
	return "fatal err";
      case APPSTA_STOP:
	return "stopped";
      case APPSTA_ERR:
	return "error";
      case APPSTA_STARTING:
	return "starting";
      case APPSTA_RUN:
	return "running";
      case APPSTA_INIT:
	return "initial";
      default:
	return "unknown";
    }
}


int licon_app_set_status(struct licon_app* app, int status)
{
    
    time_t uwtime = 0;

    switch(status){
      case APPSTA_STARTING:
		/* save the time where it was started */
		app->time_started = time(NULL);
      case APPSTA_RUN:
		app->chk_next = 0;
		uwtime = licon_time_uptime(app->time_started, 0);
		if(uwtime<app->uptime_ok){
			/* remain in starting state until app has proved running for a wile*/
			status = APPSTA_STARTING;
		} else {
			if( app->fail) fprintf(stderr, "resetting faults for %s\n", app->name);
			app->fail=0;
		}
		break;
      case APPSTA_ERR:
      case APPSTA_FATAL:
		if(!licon_time_until(app->chk_next, TIME_NOW))
			app->chk_next = licon_time_get_next_chk(app->fail, 0, app->waittime);
		uwtime = licon_time_until(app->chk_next, TIME_NOW);
		app->fail++;
		app->failtot++;
		if(app->max_fail && app->fail > app->max_fail){
			fprintf(stderr, "App check failed > max: %s\n", app->name);
			status = APPSTA_FATAL;
		}
		break;
      case APPSTA_STOP:
		app->chk_next = 0;
		app->fail = 0;
		break;
    }
    
    if(app->status != status){
		char errtxt[1024];
		errtxt[0] = '\0';

		if((app->errfile)&&((status==APPSTA_ERR)||(status==APPSTA_FATAL)))
			licon_file_read(app->errfile,errtxt, sizeof(errtxt));

		if(uwtime)
			licon_log_eventv("app", app->name, licon_app_status_text(status), 
							 "(f %d, tf %d)[%d] %s",app->fail,app->failtot, uwtime, errtxt );
		else
			licon_log_eventv("app", app->name, licon_app_status_text(status), 
							 "(f %d, tf %d) %s",app->fail,app->failtot,errtxt );

		fprintf(stderr, "App state changed: %s (%s --> %s) (%lu sec)\n",
				app->name, licon_app_status_text(app->status) ,  licon_app_status_text(status), 
				uwtime);
    }
    
    app->status = status;

    return status;
 
}

void licon_app_print(struct licon_app* app)
{
     if(!app)
        return;

     printf("app: %s \n", app->name);
    
    if(app->cmd_start)
        printf("  cmd_start:  %s\n", app->cmd_start);

    if(app->cmd_stop)
        printf("  cmd_stop : %s\n", app->cmd_stop);

    if(app->pidfile)
        printf("  pidfile  : %s\n", app->pidfile);
    
    printf("  ");
    licon_waittime_print(app->waittime);

    licon_app_print(app->next);

}

int licon_app_status(struct licon_app* app, char* buf, int maxlen)
{
    int ret = 0;
    int pid = 0;
    time_t uwtime = 0;

    if(!app)
        return snprintf(buf+ret, maxlen-ret, 
			"if name             ,  pid  , fail , failtot, status  \n");

    switch(app->status){
      case APPSTA_STARTING:
      case APPSTA_RUN:
	uwtime = licon_time_uptime(app->time_started, TIME_NOW);
	break;
      case APPSTA_ERR:
      case APPSTA_FATAL:
	uwtime = licon_time_until(app->chk_next, TIME_NOW);
	break;
    }

    pid = licon_pidfile_pid(app->pidfile);

    snprintf(buf+ret, maxlen-ret , "%s                   ", 
             app->name);
    ret += 20;
    ret += snprintf(buf+ret, maxlen-ret, ", %5d , %4d , %5d, %s (%lu)", 
                    pid, app->fail, app->failtot, 
		    licon_app_status_text(app->status), uwtime);

    if(app->ignorerr)
	ret += snprintf(buf+ret, maxlen-ret, " (ignore)");

    if(!app->enabled)
	ret += snprintf(buf+ret, maxlen-ret, " (disabled)");

    ret += snprintf(buf+ret, maxlen-ret, "\n");
    

    return ret;
}




char *licon_tun_get_pwd(void)
{
    return strdup("kamel65dyt5");
}


char *licon_tun_get_macusr(void)
{
    int mac[8];
    int retval;
    char *hwa = licon_platform_conf("liabETH");
    char user[32];
    
    if(hwa == NULL) {
        sprintf(user, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", 0, 0, 0, 0, 0, 0);
        return strdup(user);
    }

    if((retval = sscanf(hwa, "%x:%x:%x:%x:%x:%x", 
                        &mac[0], &mac[1], &mac[2], 
                        &mac[3], &mac[4], &mac[5]))==6){
        free(hwa);
        sprintf(user, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return strdup(user);
    } 

    free(hwa);

    fprintf(stderr, "error reading mac: sscanf returned %d\n", retval);

    return NULL;
}

int licon_tun_write_chap(const char *user, const char *passwd)
{
    
    return 0;

}

int licon_app_wait_check(struct licon_app *app)
{
	time_t until;

	until = licon_time_until(app->chk_next, 0);

    if(until){
		//fprintf(stderr, "until > 0 (%lu)", until);
		return 0;
    }

    return 1;
}

int licon_app_enabled_check(struct licon_app *app)
{
	if(!app->enabled){
		return 0;
    }

    return 1;
}


int licon_app_start(struct licon_app *app)
{
    
    int retval;
    int status;

    if(!app->enabled){
		return APPSTA_STOP;
    }

    if((app->status == APPSTA_STOP)&&(app->pidfile)){
	if(!licon_pidfile_check(app->pidfile)){
	    fprintf(stderr, "\"%s\" id running :: stopping\n", app->pidfile);
	    licon_pidfile_stop(app->pidfile);
	}
    }

    fprintf(stderr, "starting app: %s \n", app->name);
    switch(app->type){
      case APPTYPE_HUP:
        retval = licon_pidfile_hup(app->pidfile);
        break;
      case APPTYPE_APP:
      default:
        retval = licon_cmd_run(app->cmd_start);
        break;
        
    }
    
    if(retval!=0){
	status = APPSTA_ERR;
        fprintf(stderr, "Err: start app: %s \n", app->name);
    } else {
	status = APPSTA_STARTING;
    }

    return status;
}

int licon_app_status_all(struct licon_app *app)
{
    int next_sta;
    int status;
    
    if(!app)
	return APPSTA_NONE;

    next_sta = licon_app_status_all(app->next);

    if(app->ignorerr)
	status = APPSTA_IGNORE;
    else 
	status = app->status;
    


    if(next_sta < status)
	return next_sta;
    else
	return status;
}


int licon_app_start_all(struct licon_app *app)
{
    int status = APPSTA_NONE;
    int status_next =  APPSTA_NONE;
    
    if(!app)
        return status ;

    status_next = licon_app_start_all(app->next);

    status = licon_app_set_status(app, licon_app_start(app));

    if(status_next < status)
	status = status_next;
    
    return status;

}


/**
 * Check i all applications are running 
 * @return 
 */
int licon_app_check(struct licon_app *app)
{
    int status = APPSTA_STOP;
    
    if(!app->pidfile){
        app->fail = 0;
        return 0;
    }
    
    if(!licon_app_enabled_check(app)){
		if(app->status!=APPSTA_STOP) 
		 	licon_app_stop(app); 
		return app->status;
    }

	if(!licon_app_wait_check(app)){
		return app->status;
	}

//    fprintf(stderr, "App check: %s \n",
//	    app->name);
	
    switch(app->status){
      case APPSTA_STARTING:
      case APPSTA_RUN:
	/* application should be running */
	if(licon_pidfile_check(app->pidfile)){
	    fprintf(stderr, "App check failed for: %s (%d < %d)\n",
		    app->name, app->fail ,  app->max_fail);
	    status = APPSTA_ERR;
	} else {
	    status = APPSTA_RUN;
	}
	break;
      case APPSTA_ERR:
      case APPSTA_FATAL:
		status = licon_app_start(app);
		break;
      case APPSTA_INIT:
      case APPSTA_STOP:
		licon_app_set_status(app, APPSTA_STARTING);
		status = licon_app_start(app);
//	status = app->status;
	break;
      default:
	return APPSTA_NONE;
    }

    return  licon_app_set_status(app, status);

}

int licon_app_check_all(struct licon_app *app)
{
    int retval = 0;
    int retval_child = 0;
    
    if(!app)
        return 0;

    // Check this and child
    retval = licon_app_check(app);
    retval_child = licon_app_check_all(app->next);

    // Return the lowest status
    if(retval_child < retval)
	return retval_child;
    else 
	return retval;
}




int licon_app_stop(struct licon_app *app)
{
    if(app->cmd_stop){
        licon_cmd_run(app->cmd_stop);
    } else if (app->pidfile ){
        licon_pidfile_stop(app->pidfile);
    }


//    licon_log_event("app", app->name, "stopped");

    licon_app_set_status(app, APPSTA_STOP);
    return 0;
}

int licon_app_stop_all(struct licon_app *app)
{
    if(!app)
        return 0;

    licon_app_stop(app);

    licon_app_stop_all(app->next);
        
    return 0;
    
}

struct licon_app *licon_tun_create(const char *pppname)
{
    char name[32];
    char cmd_start[512];
    char cmd_stop[512];
    char *usr = licon_tun_get_macusr();
    char *passwd = licon_tun_get_pwd();

    snprintf(name,sizeof(name), "ppp %s", pppname);
    
    snprintf(cmd_start, sizeof(cmd_start),
             "pon %s updetach dump debug maxfail 1 user %s password %s\n",
             pppname, usr, passwd);
    
    snprintf(cmd_stop,sizeof(cmd_stop), "poff %s", pppname);
    free(usr);
    free(passwd);

    return licon_app_create(name, APPTYPE_TUN,cmd_start, cmd_stop, "/var/run/ppp4.pid");
}


struct licon_app *licon_app_testvpp_create(void)
{
    
    return licon_app_create("vppapp", APPTYPE_APP,"vppapp -i 87.48.235.162 -p 25052", NULL ,"/var/run/vppapp.pid");
}
