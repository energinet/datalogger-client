/*
 * (C) LIAB ApS 2009 - 2012
 * @ LIAB License header @
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
