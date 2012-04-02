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

#ifndef LICON_CHECK_H_
#define LICON_CHECK_H_
#define _XOPEN_SOURCE
#include <sys/time.h>


struct licon_check{
    char *cmd;
    int expect;
    int err_cnt;
    int err_max;
    int interval;
    struct timeval last; 
    struct licon_check *next;
};

struct licon_check *licon_check_create(const char *cmd, int expect, int err_max, int interval);
void licon_check_delete(struct licon_check *check);

void licon_check_reset(struct licon_check *check);

int licon_check_do(struct licon_check *check);

int licon_check_status(struct licon_check *check, char* buf, int maxlen);





#endif /* LICON_CHECK_H_ */
