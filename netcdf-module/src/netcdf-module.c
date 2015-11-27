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

#include "module_base.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h>
#include <lognetcdf.h>
#include <assert.h>

struct module_object {
	struct module_base base;
	char *filepath;
	int file_ct;
	struct lognetcdf *db;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	struct module_event* fifo_list;
};

struct module_object* module_get_struct(struct module_base *module) {
	return (struct module_object*) module;
}

int module_fifo_push(struct module_object *module, struct module_event* event) {
	struct module_event* ptr = module->fifo_list;
	int count = 0;

	if (!ptr) {
		module->fifo_list = event;
		goto out;
	}

	count++;

	while (ptr->next) {
		ptr = ptr->next;
		count++;
	}

	ptr->next = event;

	out:

	return count;

}

struct module_event* module_fifo_pop(struct module_object *module) {
	struct module_event* retptr = NULL;
	struct module_event* fifo_list = NULL;

	retptr = module->fifo_list;

	if (retptr) {
		module->fifo_list = retptr->next;
		retptr->next = NULL;
	}

	return retptr;
}

int event_handler_all(EVENT_HANDLER_PAR) {
	struct module_object* this = module_get_struct(handler->base);
	int retval = 0;

	if (pthread_mutex_lock(&this->mutex) != 0)
		return -EFAULT;

	retval = module_fifo_push(this, module_event_copy(event));

	PRINT_MVB(handler->base, "push log %s.%s count %d\n", event->source->name, event->type->name, retval);

	pthread_cond_broadcast(&this->cond);
	pthread_mutex_unlock(&this->mutex);

	return 0;

}

int module_write_log(struct module_object* this, struct module_event* event) {
	struct uni_data *data = event->data;
	//int txt_size = 512;
	//char entry_txt[txt_size];
	char ename[64];
	int eid;
	int ptr = 0;
	int retval = 0;
	float fdata;

	sprintf(ename, "%s.%s", event->source->name, event->type->name);

	eid = lognetcdf_etype_get_eid(this->db, ename);

	if (eid == -1) {
		lognetcdf_etype_add(this->db, ename, event->type->hname,
				event->type->unit, "m");
		eid = lognetcdf_etype_get_eid(this->db, ename);
	} else if (eid < -1)
		return -1;

	fdata = uni_data_get_value(data);

	PRINT_MVB(&this->base,"Module event %s.%s writing (eid %d) value %f (len%d)\n", event->source->name, event->type->name, eid, fdata, ptr);

	retval = lognetcdf_evt_add(this->db, eid, &event->time, &fdata);

	if(retval){
		PRINT_ERR("lognetcdf_evt_add (%s.%s) returned error: %d", event->source->name, event->type->name, retval);
	}

	return retval;

}

struct event_handler handlers[] = { { .name = "log",
		.function = event_handler_all }, { .name = "" } };
struct event_type events[] = { { .name = "fault" }, { .name = "" } };

int start_netcdffile(XML_START_PAR) {
	struct module_object* this = module_get_struct(ele->parent->data);
	const char *netcdffile = get_attr_str_ptr(attr, "file");

	if (!this)
		return 0;

	if (this->filepath) {
		PRINT_ERR("filepath allready loaded");
		return -1;
	}

	if (!netcdffile) {
		PRINT_ERR("no text");
		return -EFAULT;
	}

	this->filepath = strdup(netcdffile);

	this->db = lognetcdf_create(this->base.verbose);

	lognetcdf_open(this->db, this->filepath, this->base.verbose);

	if (!this->db)
		return -EFAULT;

	return 0;
}

int start_netcdflog(XML_START_PAR) {
	struct modules *modules = (struct modules*) data;
	struct module_base* this = ele->parent->data;
	struct event_handler* handler = NULL;

	if (!this)
		return 0;

	handler = event_handler_get(this->event_handlers, "log");

	assert(handler);

	return module_base_subscribe_event(this, modules->list, get_attr_str_ptr(
			attr, "event"), FLAG_LOG, handler, attr);
}

struct xml_tag module_tags[] = { { "netcdf", "module", start_netcdffile, NULL,
		NULL }, { "log", "module", start_netcdflog, NULL, NULL }, { "", "",
		NULL, NULL, NULL } };

char* filepath_cr() {
	char timestamp[20] = { 0 };
	char *tmp;
	char *output = NULL;
	char prefix[12] = "/mnt/log-", suffix[4] = ".nc";
	time_t rawtime = time(0);
	struct tm *now = localtime(&rawtime);

	tmp = (char *) malloc(50);

	if (rawtime != -1) {
		strftime(timestamp, 16, "%y%m%d_%H%M%S", now);
		strcat(timestamp, suffix);
		strcpy(tmp, prefix);
		strcat(tmp, timestamp);
		output = strdup(tmp);
	}

	free(tmp);

	return output;
}

int t_check() {
	time_t tv;
	int tv_int;
	tv = time(NULL);
	tv_int = (int)tv;
	return tv_int;
}

int module_init(struct module_base *module, const char **attr) {
	struct module_object* this = module_get_struct(module);
	int retval;

	const char *file = get_attr_str_ptr(attr, "file");
	/* struct event_handler *event_handler; */

	if (file) {
		this->filepath = strdup(file);
	} else {
		this->filepath = strdup(filepath_cr());

		this->file_ct = t_check() + 3600;
		printf("Creating: %s - file_ct = %d\n", this->filepath,this->file_ct);
	}

	this->db = lognetcdf_create(this->base.verbose);

	lognetcdf_open(this->db, this->filepath, this->base.verbose);

	pthread_cond_init(&this->cond, NULL);
	pthread_mutex_init(&this->mutex, NULL);

}

void reopen(struct module_object *this) {
	this->file_ct = t_check() + 3600;
	lognetcdf_close(this->db);
	free(this->filepath);
	this->filepath = strdup(filepath_cr());
	lognetcdf_open(this->db, this->filepath, this->base.verbose);
}

void* module_loop(void *parm) {
	struct module_object *this = module_get_struct(parm);
	struct module_base *base = (struct module_base *) parm;
	int retval = 0;
	base->run = 1;

	while (base->run) {
		struct module_event* event = NULL;
		if (retval = pthread_cond_wait(&this->cond, &this->mutex))
			break;

		while ((event = module_fifo_pop(this)) != NULL) {
			if((this->file_ct) < t_check()) {
				reopen(this);
			}
			pthread_mutex_unlock(&this->mutex);
			PRINT_MVB(base, "logging %p %p %s\n", event, event->type, event->type->name);
			while (module_write_log(this, event) < 0) {
				PRINT_ERR("logging error %s recreating NetCDF file...\n", event->type->name);
				reopen(this);
			}
			module_event_delete(event);
			pthread_mutex_lock(&this->mutex);
		}
		pthread_mutex_unlock(&this->mutex);
	}

	PRINT_MVB(base, "loop returned");

	return NULL;

}

void module_deinit(struct module_base *module) {
	struct module_object* this = module_get_struct(module);

	pthread_cond_destroy(&this->cond);
	pthread_mutex_destroy(&this->mutex);

	lognetcdf_close(this->db);
	lognetcdf_destroy(this->db);

	return;
}

struct event_handler *module_subscribe(struct module_base *module,
		struct module_base *source, struct event_type *event_type,
		const char **attr) {
	struct module_object *this = module_get_struct(module);
	struct event_handler* handler = NULL;
	char *type;
	char ename[64];
	if (strcmp(event_type->name, "fault") == 0)
		return NULL;

	sprintf(ename, "%s.%s", source->name, event_type->name);
	PRINT_MVB(module, "subscribe %s", ename);

	if (event_type->flags & FLAG_SHOW)
		type = get_attr_str_ptr(attr, "type");
	else
		type = "hide";

	lognetcdf_etype_add(this->db, ename, event_type->hname, event_type->unit,
			type);

	handler = event_handler_get(this->base.event_handlers, "log");
	printf("-->%p %s\n", handler, type);
	return handler;

}

struct module_type module_type = { .name = "netcdflogger",
		.mtype = MTYPE_DATALOG, .subtype = SUBTYPE_NONE,
		.xml_tags = module_tags, .handlers = handlers,
		.type_struct_size = sizeof(struct module_object), };

struct module_type *module_get_type() {
	return &module_type;
}
