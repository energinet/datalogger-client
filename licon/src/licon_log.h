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

#ifndef LICON_LOG_H_
#define LICON_LOG_H_

#include <time.h>
#include <sys/time.h> 
#include <syslog.h>

struct licon_log{
    struct timeval tv;
    char *text;
    struct licon_log *next;
};

#define PRINT_LOG(priority, str,arg...) licon_sysdbg_log(priority, str"\n", ## arg)

void licon_sysdbg_set(int set);

void licon_sysdbg_log(int priority, const char *format,...);


struct licon_log *licon_log_create(const char *type, const char *name, const char *event);
struct licon_log *licon_log_createv(const char *type, const char *name, const char *event, const char *format, ...);

void licon_log_delete(struct licon_log *log);

void licon_log_list_add(struct licon_log *new);
//struct licon_log *licon_log_list_add(struct licon_log *first, struct licon_log *new);


void licon_log_event(const char *type, const char *name, const char *event);

void licon_log_eventv(const char *type, const char *name, const char *event, const char *format, ...);

struct licon_log *licon_log_rgetfirst(void);
//struct licon_log *licon_log_first(void);
//void licon_log_rem(struct licon_log *log);

#endif /* LICON_LOG_H_ */
