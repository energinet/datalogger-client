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
 
#ifndef LOGNETCDF_H_
#define LOGNETCDF_H_

#include <time.h>
#include <sys/time.h>

struct lognetcdf {
    int ncid;
    int eid_max;
    int debug_level;
};

/* create lognetcdf */
struct lognetcdf *lognetcdf_create(int debug_level);

/* open netcdf file */
int lognetcdf_open(struct lognetcdf *netcdf, const char *filepath, int debug_level);

/* close database file */
void lognetcdf_close(struct lognetcdf *db);

/* destroy lognetcdf */
void lognetcdf_destroy(struct lognetcdf *db);

/* Add event */
int lognetcdf_evt_add(struct lognetcdf *db, int eid, struct timeval *time, const float* value);

/* Add event type */
int lognetcdf_etype_add(struct lognetcdf *db, char *name, char *hname, 
                    char *unit, char *type);

/* Get eid */
int lognetcdf_etype_get_eid(struct lognetcdf *db, const char *ename);

#endif /* LOGNETCDF_H_ */
