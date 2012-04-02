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

#ifndef ENDATA_H_
#define ENDATA_H_

#include <sys/time.h>



enum endata_type{
	ENDATA_NONE = -1,
	ENDATA_ALL = 0,
	ENDATA_TEMPERATURE,
	ENDATA_WINDSPEED,
	ENDATA_WINDDIRECTION,
	ENDATA_HUMIDITY,
	ENDATA_INDIRECTRADIATION,
	ENDATA_DIRECTRADIATION,
	ENDATA_SPOTPRICE,
	ENDATA_CONSUMPTION_PROGNOSIS,
	ENDATA_THERMAL_PRODUCTION_PROGNOSIS,
	ENDATA_VINDPOWER_PROGNOSIS,
	ENDATA_IMPORT_PROGNOSIS,
	ENDATA_IMBALACEPRICES,
	ENDATA_CO2,
	ENDATA_GRID_TARIF,
	ENDATA_TRADER,
	ENDATA_ALTITUDE,
	ENDATA_AZIMUTH,                   
};


enum endata_lang{
	ENDATA_LANG_DK,
	ENDATA_LANG_UK
};

enum endata_sort{
	ENDATA_SORT_NONE = -1,
	ENDATA_SORT_MINFIRST = 0,
	ENDATA_SORT_MAXFIRST,
/*	ENDATA_SORT_TIME,*/
/*	ENDATA_SORT_TIMEREV,*/
};


struct endata_bitem{
	float value;
	time_t time;
	struct endata_bitem *next;
};


enum endata_lang endata_lang_get(const char *str);

struct endata_bitem *endata_bitem_create(float value, time_t i_time);

struct endata_bitem *endata_bitem_add(struct endata_bitem *list, 
									  struct endata_bitem *new);

struct endata_bitem *endata_bitem_rem(struct endata_bitem *list, 
									  struct endata_bitem *rem);


void endata_bitem_delete(struct endata_bitem *item);

struct endata_bitem *endata_bitem_value_sort(struct endata_bitem *list, enum endata_sort stype);

struct endata_bitem *endata_bitem_value_get(struct endata_bitem *item, float maxval);

void endata_bitem_period(struct endata_bitem *item, time_t *t_begin_, time_t *t_end_, time_t *t_interval_);

int endata_bitem_subsequent(struct endata_bitem *item, struct endata_bitem *list, time_t t_interval);

void endata_bitem_print(struct endata_bitem *item);

struct endata_block{
	time_t t_begin;
	time_t t_end;
	time_t t_interval;
	enum endata_type type;
	char *unit;
	struct endata_bitem *list;
	struct endata_block *next;
};

struct endata_block *endata_block_create(enum endata_type type, const char *unit);

struct endata_block *endata_block_add(struct endata_block *list, struct endata_block *new);

struct endata_block *endata_block_rem(struct endata_block *list, struct endata_block *rem);

void endata_block_delete(struct endata_block *block);

void endata_bitem_period(struct endata_bitem *item, time_t *t_begin_r, time_t *t_end_r, time_t *t_interval_r);

struct endata_block *endata_block_gettype(struct endata_block *list,enum endata_type type );

void endata_block_print(struct endata_block *block);



struct endata_block * endata_get(int inst_id, int day , enum endata_lang language, enum endata_type select,
								 const char *address, int dbglev);


#endif /* ENDATA_H_ */

