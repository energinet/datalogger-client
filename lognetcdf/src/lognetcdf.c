/*
 * (C) LIAB ApS 2009 - 2012
 * @ LIAB License header @
 */

#include "lognetcdf.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <netcdf.h>

#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s: "str"\n",__FUNCTION__, ## arg)
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}

/* Handle NetCDF errors by printing an error message */
#define ERRCODE 2
#define NETCDF_DBG(cond,e) {if(cond){printf("NetCDF error: %s\n", nc_strerror(e));}}
#define NETCDF_ERR(e) printf("NetCDF error: %s\n", nc_strerror(e))

#define GET_ENAME 0
#define GET_UNIT 1


struct lognetcdf_list {
	int id;
	char *id_name;
	char *unit;
	struct lognetcdf_list *next;
}*head;

#define NETCDF_TABLE_NAME_EVENT_TYPE "event_type"

/* create lognetcdf */
struct lognetcdf *lognetcdf_create(int debug_level) {
	int retval;
	struct lognetcdf *db = malloc(sizeof(*db));
	assert(db);

	head = NULL;

	db->debug_level = debug_level;

	PRINT_DBG(debug_level, "DB has adr %p",db);

	return db;
}

/* open netcdf file */
int lognetcdf_open(struct lognetcdf *db, const char *filepath, int debug_level) {
	int retval;

	db->debug_level = debug_level;

	PRINT_DBG(debug_level, "Creating netcdf file %s",filepath);

	retval = nc_create(filepath, NC_CLOBBER, &db->ncid);

	if (retval < 0) {
		PRINT_ERR("Error in nc_create");
		NETCDF_ERR(retval);
		goto err_out;
	}

	retval = nc_enddef(db->ncid);

	if (retval < 0) {
		NETCDF_ERR(retval);
		goto err_out;
	}

	return db;

	err_out: nc_close(db->ncid);

	return -1;

}

/* close netcdf file */
void lognetcdf_close(struct lognetcdf *db) {
	PRINT_DBG(db->debug_level, "Closing NetCDF file");
	nc_close(db->ncid);
}

/* destroy lognetcdf */
void lognetcdf_destroy(struct lognetcdf *db) {
	assert(db);
	PRINT_DBG(db->debug_level, "Destroy lognetcdf");
	free(db);
}

/* Get ename */
const char* lognetcdf_etype_get(struct lognetcdf *db, int eid, char get) {
	PRINT_DBG(db->debug_level, "Get ename for eid = %d", eid);

	const char *ename = NULL;
	const char *unit = NULL;

	if ((eid == 0) || (eid == 1)) {
		if (eid == 0) {
			ename = "POSIX";
		} else {
			ename = "uSec";
		}
	} else {
		struct lognetcdf_list *cur_ptr;
		cur_ptr = head;

		if (cur_ptr != NULL) {
			while (cur_ptr != NULL) {
				if (eid == cur_ptr->id) {
					ename = cur_ptr->id_name;
					unit = cur_ptr->unit;
					break;
				}
				cur_ptr = cur_ptr->next;
			}
		}
	}

	if (get == GET_ENAME) {
		PRINT_DBG(db->debug_level, "ename is %s", ename);
		return ename;
	} else {
		if(unit == NULL) {
			unit = "";
		} else if(strlen(unit) == 0) {
			unit = "";
		}
		PRINT_DBG(db->debug_level, "unit is %s", unit);
		return unit;
	}
}

/* init netcdf file */
int lognetcdf_init_ncfile(struct lognetcdf *db, int *varid) {
	int retval, i;
	int time_dimid, value_dimid;
	int dimids[2];
	const char* ename_str;
	const char* unit_str;
	char cols_str[256];
	char ename_unit_str[256];

	PRINT_DBG(db->debug_level,"Creating Variables (eid_max: %d)\n",  db->eid_max);

	if ((retval = nc_redef(db->ncid))) {
		PRINT_ERR("Error in nc_redef");
		NETCDF_ERR(retval);
		return retval;
	}

	if ((retval = nc_def_dim(db->ncid, "time", NC_UNLIMITED, &time_dimid))) {
		PRINT_ERR("Error in nc_def_dim('time')");
		NETCDF_ERR(retval);
		return retval;
	}

	if ((retval = nc_def_dim(db->ncid, "value", db->eid_max + 1, &value_dimid))) {
		PRINT_ERR("Error in nc_def_dim('value')");
		NETCDF_ERR(retval);
		return retval;
	}

	dimids[0] = time_dimid;
	dimids[1] = value_dimid;

	if ((retval = nc_def_var(db->ncid, "data", NC_INT, 2, dimids, varid))) {
		PRINT_ERR("Error in nc_def_var('date')");
		NETCDF_ERR(retval);
		return retval;
	}

	/* Define and add attributes for cols */
	for (i = 0; i < (db->eid_max + 1); i++) {
		ename_str = lognetcdf_etype_get(db, i, GET_ENAME);
		sprintf(cols_str, "col%d", i);
		unit_str = lognetcdf_etype_get(db, i, GET_UNIT);
		sprintf(ename_unit_str,"%s,%s",ename_str,unit_str);
		if ((retval = nc_put_att_text(db->ncid, *varid, cols_str, strlen(
				ename_unit_str), ename_unit_str))) {
			PRINT_ERR("Error in nc_put_att_text");
			NETCDF_ERR(retval);
			return retval;
		}
	}

	if ((retval = nc_enddef(db->ncid))) {
		PRINT_ERR("Error in nc_enddef %d", db->ncid );
		NETCDF_ERR(retval);
		return retval;
	}

	if ((retval = nc_sync(db->ncid))) {
		PRINT_ERR("Error in nc_sync %d", db->ncid );
		NETCDF_ERR(retval);
		return retval;
	}

	return 0;
}

/* Add event */
int lognetcdf_evt_add(struct lognetcdf *db, int eid, struct timeval *time,
		const float* value) {
	int i, varid, dimid, retval;

	size_t lenp, indexp[] = { 0, 0 };
	int data_out[1024];
	int old_val[3];
	int old_val_cnt = -1;

	PRINT_DBG(db->debug_level,"Add event - eid = %d, tv_sec = %d, tv_usec = %d\n value = %f\n",eid, time->tv_sec, time->tv_usec,*value);

	/* If no data variable exist (new file) */
	if ((retval = nc_inq_varid(db->ncid, "data", &varid))) {
		NETCDF_DBG(db->debug_level,retval);

		if ((retval = lognetcdf_init_ncfile(db, &varid))) {
			PRINT_ERR( "Error in lognetcdf_init_ncfile: eid = %d ", eid);
			NETCDF_ERR(retval);
			return retval;
		}
		lenp = 0;
	} else {
		if ((retval = nc_inq_unlimdim(db->ncid, &dimid))) {
			PRINT_ERR( "Error in nc_inq_unlimdim");
			NETCDF_ERR(retval);
			return retval;
		}

		if ((retval = nc_inq_dimlen(db->ncid, dimid, &lenp))) {
			PRINT_ERR("Error in nc_inq_dimlen");
			NETCDF_ERR(retval);
			return retval;
		}

		/* Load the last 3 timestamps */
		indexp[0] = lenp - 1;
		indexp[1] = 1;
		for (i = 0; i < 3; i++) {
			if (lenp > 3) {
				indexp[0] = lenp - (1 + i);
			}
			if ((retval = nc_get_var1_int(db->ncid, varid, indexp, &old_val[i]))) {
				PRINT_ERR( "Error in nc_get_var1_int");
				NETCDF_ERR(retval);
				return retval;
			}
		}
	}

	data_out[0] = time->tv_sec;
	data_out[1] = time->tv_usec;
	data_out[eid] = (int) (*value * 1000);

	/* Check if one of the 3 old timestamps matches the new one */
	if (lenp != 0) {
		for (i = 0; i < 3; i++) {
			old_val_cnt = -1;
			if (data_out[1] == old_val[i]) {
				old_val_cnt = i;
				break;
			}
		}
	}

	if (-1 == old_val_cnt) {
		indexp[0] = lenp;
		/* Write new timestamp */
		for (i = 0; i < 2; i++) {
			indexp[1] = i;
			if ((retval
					= nc_put_var1_int(db->ncid, varid, indexp, &data_out[i]))) {
				fprintf(stderr, "Error in nc_put_var1_int");
				NETCDF_ERR(retval);
				return retval;
			}
		}
	} else if (old_val_cnt >= 0) {
		indexp[0] = lenp - old_val_cnt - 1;
	}

	indexp[1] = eid;
	if ((retval = nc_put_var1_int(db->ncid, varid, indexp, &data_out[eid]))) {
		fprintf(stderr, "Error in nc_put_var1_int");
		NETCDF_ERR(retval);
		return retval;
	}

	if ((retval = nc_sync(db->ncid))) {
		fprintf(stderr, "Error in nc_sync");
		NETCDF_ERR(retval);
		return retval;
	}

	return 0;
}

/* Get eid */
int lognetcdf_etype_get_eid(struct lognetcdf *db, const char *ename) {
	int ptr = 0;
	int result = -1;

	PRINT_DBG(db->debug_level, "Get eid for %s %d %d", ename, ptr, result);

	struct lognetcdf_list *cur_ptr;

	cur_ptr = head;

	if (cur_ptr != NULL) {
		while (cur_ptr != NULL) {
			if (strcmp(ename, cur_ptr->id_name) == 0) {
				result = cur_ptr->id;
				break;
			}
			cur_ptr = cur_ptr->next;
		}
	}

	PRINT_DBG(db->debug_level, "eid is %d", result);

	return result;
}

/* Add event type */
int lognetcdf_etype_add(struct lognetcdf *db, char *ename, char *hname,
		char *unit, char *type) {
	struct lognetcdf_list *temp1, *temp2;
	char time_str[32];
	struct timeval tv;
	int eid = lognetcdf_etype_get_eid(db, ename), eid_temp = 2;
	int ptr = 0;

	temp1 = (struct lognetcdf_list *) malloc(sizeof(struct lognetcdf_list));

	/* Copying the Head location into another node. */
	temp2 = head;
	temp1->id_name = strdup(ename);
	if(unit != NULL) {
		temp1->unit = strdup(unit);
	}

	if (head == NULL) {
		/* If List is empty we create First Node. */
		temp1->id = eid_temp;
		head = temp1;
		head->next = NULL;
	} else {
		/* Traverse down to end of the list. */
		eid_temp++;
		while (temp2->next != NULL) {
			temp2 = temp2->next;
			eid_temp++;
		}
		temp1->id = eid_temp;
		/* Append at the end of the list. */
		temp1->next = NULL;
		temp2->next = temp1;
	}

	PRINT_DBG(db->debug_level, "Adding etype: %s, eid = %d\n", ename,eid_temp);

	db->eid_max = eid_temp;

	return 0;
}
