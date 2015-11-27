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

#include "encontrol_simp.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>
#include <signal.h>
#include <endata.h>


struct endata_block *encontrol_simp_endata_get(int inst_id, enum endata_type endata_type, enum endata_sort stype, int day, int dbglev)
{
	struct endata_block *block_list =  endata_get(inst_id, day , ENDATA_LANG_UK, endata_type,NULL,dbglev);
	if(!block_list){
		PRINT_ERR("endata_get returned NULL");
		return NULL;
	}

	if(dbglev)
		endata_block_print(block_list);

	struct endata_block *block = endata_block_gettype(block_list,endata_type);
	if(!block){
		PRINT_ERR("block dit not contain ENDATA_SPOTPRICE");
		return NULL;
	}
	
	block_list = endata_block_rem(block_list, block);

	endata_block_delete(block_list);

	block->list = endata_bitem_value_sort(block->list, stype);

	if(dbglev) 
		endata_block_print(block);

	return block;
}

int encontrol_simp_offhours(struct logdb *logdb, const char *varnane, time_t now, int dbglev)
{
	int eid = logdb_etype_get_eid(logdb, varnane); 
	int count, off_count;
	char sql[512];
	
	sprintf(sql, "SELECT COUNT(*) FROM event_log WHERE eid=%d AND time > %ld", eid, now-(24*60*60));

	logdb_get_first_int(logdb, sql, 0, &count);

	sprintf(sql, "SELECT SUM(value<%d) FROM event_log WHERE eid=%d AND time > %ld", 300/*min power*/, eid, now-(24*60*60));

	logdb_get_first_int(logdb, sql, 0, &off_count);



	if(count){
		PRINT_DBG(dbglev, "count %d on_count %d, = %f", count,off_count,(off_count/((float)count)));
		return (24*(off_count/((float)count)))/2;
	}else {
			PRINT_DBG(dbglev, "count %d on_count %d, = inf", count,off_count);
		return -1;
	}

}

 
int encontrol_simp_plan_set(struct endata_bitem *off_bitems, time_t t_interval,time_t t_begin,time_t t_end, int dbglev)
{
	struct cmddb_plan *off_plan = NULL;
	struct endata_bitem *ptr = off_bitems;
	time_t now = time(NULL);

	while(ptr){
		time_t t_start = ptr->time;
		time_t t_end   = ptr->time+t_interval-1;
		
		if(t_start > now)
			off_plan = cmddb_plan_add(off_plan , cmddb_plan_create(CMD_ROWID_UNDEF, t_start, "1"));
		if(t_end > now)
			off_plan = cmddb_plan_add(off_plan , cmddb_plan_create(CMD_ROWID_UNDEF, t_end, "0"));
		
		PRINT_DBG(dbglev, "1 at %s", ctime(&t_start));
		PRINT_DBG(dbglev, "0 at %s", ctime(&t_end));
		
		ptr = ptr->next;	
		
	}
	
	if(t_begin < now)
		t_begin = now;

	return cmddb_plan_set(off_plan, "pws_relay1", t_begin, t_end-1);
	
}

int encontrol_simp_calc_all(struct endata_block *block, int off_hours , int dbglev )
{
	int retval = 0;
	struct endata_bitem *off_bitems = NULL;
	/* Make sure that there is no subsequent off hours */
	
	struct endata_bitem *ptr = block->list;
	while(ptr){
		if(!endata_bitem_subsequent(ptr, off_bitems, block->t_interval)
		   &&(block->t_begin != ptr->time)){
			off_bitems = endata_bitem_add(off_bitems ,endata_bitem_create(ptr->value, ptr->time));
			if(--off_hours<=0)
				break;
		}
		
		ptr = ptr->next;
	}
	
	if(dbglev)
		endata_bitem_print(off_bitems);

	retval = encontrol_simp_plan_set(off_bitems, block->t_interval, block->t_begin, block->t_end, dbglev);

	endata_bitem_delete(off_bitems);
	
	return retval;

}


struct encontrol_simp_type{
	const char *typename;
	enum endata_type endata_type;
	enum endata_sort stype;
	int max_offhour;
};

struct encontrol_simp_type optitypes[] = {
	{ "alwayson",  ENDATA_SPOTPRICE, ENDATA_SORT_NONE , 0},
	{ "spotprice", ENDATA_SPOTPRICE, ENDATA_SORT_MAXFIRST , 3},
	{ "windpower", ENDATA_VINDPOWER_PROGNOSIS, ENDATA_SORT_MINFIRST ,3 },
	{ "CO2",       ENDATA_CO2, ENDATA_SORT_MAXFIRST ,3 },
	{ "extern",    ENDATA_NONE, ENDATA_SORT_NONE, -1 },
	{ NULL,       ENDATA_NONE, ENDATA_SORT_NONE , 0  }
};

int encontrol_simp_calc(int inst_id, struct logdb *logdb, const char *calctype, int dbglev )
{
	int i = 0; 
	enum endata_type endata_type = ENDATA_NONE;
	enum endata_sort stype	= ENDATA_SORT_NONE;
	struct endata_block *block = NULL;
	time_t now  = time(NULL);
	struct encontrol_simp_type *optitype = NULL;
	int day = 0;
	int off_hours;
	int retval = 0;
	
	while(optitypes[i].typename){
		if(strcmp(optitypes[i].typename, calctype)==0){
			optitype = optitypes + i;
			endata_type = optitypes[i].endata_type;
			stype = optitypes[i].stype;
			break;
		}
		i++;
	}

	if(!optitype){
		PRINT_ERR("Type '%s' unknown", calctype);
		return -1;
	}
	

	if(optitype->max_offhour < 0)
		return 0; 

	/* calc day */
	if((now%(24*60*60))>(60*60*13))
		day = 1;

	PRINT_DBG(dbglev, "Running schedule update for '%s' day: %d int_id %d\n", calctype, day, inst_id);

	/* get energinet data */
	if((block = encontrol_simp_endata_get(inst_id, endata_type, stype, day, dbglev))==NULL){
		PRINT_ERR("Error: no energinet data");
		return 0;
	}

	/* calc off hours */
	
	off_hours = encontrol_simp_offhours(logdb, "acc.pwhp", now,dbglev);
	
	if(off_hours > optitype->max_offhour || off_hours == 0)
		off_hours = optitype->max_offhour;
	
	retval =  encontrol_simp_calc_all(block, off_hours, dbglev );


	endata_block_delete(block);

	return retval;
}
