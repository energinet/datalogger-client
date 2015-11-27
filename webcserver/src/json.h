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



#ifndef JSON_H_
#define JSON_H_


#include <time.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

enum jsontype{
	JSNONE,
	JSOBJECT,
	JSARRAY,
	JSNUMBER,
	JSTEXT,
	JSDATA,
};

struct json{
	/**
	 * Name of the obj/array/value */
	char *name;
	/**
	 * The value if the type is str, int og time*/
	char *value;
	/**
	 * The type */
	enum jsontype type;
	/**
	 * Objects included into this */
	struct json *childs;
	/**
	 * Next json object */
	struct json *next;
};

struct json *json_create_str(const char *name, const char *value);

struct json *json_create_int(const char *name, int value);

struct json *json_create_time(const char *name, time_t value);

struct json *json_create_obj(const char *name);

struct json *json_create_array(const char *name);

struct json *json_list_add(struct json *list, struct json *new);

struct json *json_add_child(struct json *parent, struct json *child);

void json_delete(struct json *rem);

struct json *json_parse(struct json *lst, const char *str, int *iret, int maxlen);

void json_print(FILE *stream, struct json *json, int inArray);

struct json *json_get_childs(struct json *lst, const char *name);

const char *json_get_value(struct json *lst, const char *name);

#endif /* JSON_H_ */
