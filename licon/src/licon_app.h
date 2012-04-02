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

#ifndef LICON_APP_H_
#define LICON_APP_H_

#include <time.h>
#include <sys/time.h>
#include "licon_util.h"

enum{
    APPTYPE_APP,
    APPTYPE_TUN,
    APPTYPE_HUP,
};

enum{
    APPSTA_FATAL = -2,
    APPSTA_ERR = -1,
    APPSTA_INIT = 0, 
    APPSTA_STOP,
    APPSTA_STARTING,
    APPSTA_RUN,
    APPSTA_IGNORE,
    APPSTA_NONE,
};

struct licon_app{
    char *name;
    int type;
    int fail;
    int max_fail;
    int failtot;
    int status;
    time_t uptime_ok;
    time_t chk_next;
    time_t time_started;
    int enabled;
    char *cmd_start;
    char *cmd_stop;
    char *pidfile;
	char *errfile;
    int ignorerr;
    struct licon_waittime *waittime;
    int dbglev;
    struct licon_app* next;
};
struct licon_app *licon_app_create(const char* name, int type, const char *cmd_start, 
                                   const char *cmd_stop, const char *pidfile);


struct licon_app *licon_app_add(struct licon_app *first, struct licon_app *new);


/**
 * Get a pointer to an application object 
 * @param list The interface list
 * @param name The name to search for
 * @return a pointer to the interface object or NULL if not found */
struct licon_app *licon_app_get(struct licon_app * list, const char* name);



void licon_app_print(struct licon_app* app);
int licon_app_status(struct licon_app* app, char* buf, int maxlen);
struct licon_app *licon_tun_create(const char *pppname);
struct licon_app *licon_app_testvpp_create(void);//Debug.... To be removed

int licon_app_status_all(struct licon_app *app);
int licon_app_check_all(struct licon_app *app);

int licon_app_start(struct licon_app *app);
int licon_app_stop(struct licon_app *app);

int licon_tun_start(struct licon_app *app);
int licon_tun_stop(struct licon_app *app);

int licon_app_start_all(struct licon_app *lst);
int licon_app_stop_all(struct licon_app *lst);



#endif /*LICON_APP_H_*/

