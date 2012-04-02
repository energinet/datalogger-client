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

#ifndef LICON_UTIL_H_
#define LICON_UTIL_H_

#include <time.h>
#include <sys/time.h>

#define TIME_NOW (0)
#define USE_WAITS -1999
#define NO_WAIT 0

struct licon_waittime{
    int *waittimes;
    int size;
};

int licon_file_read(const char *path, char* buf, size_t size);

int licon_pidfile_pid(const char *pidfile);

int licon_pidfile_check(const char *pidfile);

int licon_pidfile_stop(const char *pidfile);

int licon_pidfile_hup(const char *pidfile);

int licon_cmd_run(const char *cmd);

char *licon_strdup(const char *str);

char *licon_defdup(const char *str, const char *def);

char *licon_platform_conf(const char *name);

time_t licon_time_until(time_t timeout, time_t now);

void licon_waittime_print(struct licon_waittime *waittime);

time_t licon_time_uptime(time_t tstart, time_t now);

time_t licon_time_get_next_chk(int errcnt, time_t now, struct licon_waittime *waittime);


struct licon_waittime *licon_waittime_create(const char *str);

void licon_waittime_delete(struct licon_waittime *waittime);


#endif /* LICON_UTIL_H_ */
