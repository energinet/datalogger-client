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

#ifndef PIDFILE_H_
#define PIDFILE_H_

#include <stdio.h>

enum pidfile_mode{
    PMODE_EXIT,
    PMODE_RETURN,
    PMODE_HUP,    
    PMODE_KILL,
};

struct pidfile{
    char *path;
    int fd;
    enum pidfile_mode mode;
    int dbglev;
};

int pidfile_check(const char *pidfile);


struct pidfile* pidfile_create(const char *path, enum pidfile_mode mode, int dbglev);

void pidfile_delete(struct pidfile *pidfile);


#endif /* PIDFILE_H_ */
