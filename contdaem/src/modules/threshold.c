/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2011 LIAB ApS <info@liab.dk>
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
#include <unistd.h> //sleep
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/input.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <pthread.h> 

/**
 * @ingroup modules 
 * @{
 */

/**
 * @defgroup modules_threshold Module for handeling thresholds 
 * @{
 *  
 * Module for handeling threshold.\n
 * <b>Type name: threshold </b>\n
 * <b>Attributes:</b>\n
 * - \b config: The configuration name @ref sgconf and @ref dinconf\n
 * .
 * <b> Tags: </b>
 */

/**
 * Threshold type
 * @ private 
 */
enum thrt_type {
	THRT_THRH, THRT_HYST, THRT_BOUND,
};

/**
 * Threshold object struct 
 */
struct thrhld {
	float min_thr;
	float max_thr;
	int state;
	int min_out;
	int max_out;
	struct event_type *event_type;
	struct thrhld *next;
};

/**
 * Threshold module object
 */
struct thrhsmod_object {
	/**
	 * Module base*/
	struct module_base base;
	/**
	 * Threshold list */
	struct thrhld *thrhs;
};

struct thrhsmod_object* module_get_struct(struct module_base *module) {
	return (struct thrhsmod_object*) module;
}

float get_attr_float(const char **attr, const char *id, float rep_value) {
	int attr_no;
	float ret;
	char *format;

	attr_no = get_attr_no(attr, id);

	if (attr_no < 0) {
		return rep_value;
	}

	if (sscanf(attr[attr_no], "%f", &ret) != 1) {
		printf("Attribute %s could not be red: %s (format %s)\n", id,
				attr[attr_no], format);
		return rep_value;
	}
	printf("get_attr_float %f\n", ret);

	return ret;
}

/**
 * Create threshold 
 */
struct thrhld * thrhld_create(struct thrhsmod_object* module, const char **attr) {

	struct thrhld *new = malloc(sizeof(*new));
	assert(new);
	memset(new, 0, sizeof(*new));

	if (get_attr_str_ptr(attr, "thr")) {
		new->max_thr = get_attr_float(attr, "thr", 1);
		new->max_thr = new->min_thr;
	} else {
		new->max_thr = get_attr_float(attr, "max", 1);
		new->min_thr = get_attr_float(attr, "min", 2);
	}

	new->min_out = get_attr_float(attr, "vmin", 0);
	new->max_out = get_attr_float(attr, "vmax", 1);

	new->state = get_attr_float(attr, "default", 0);

	return new;

}

/**
 * Add modbus action to list
 */
struct thrhld * thrhld_add(struct thrhld* first, struct thrhld*new) {
	struct thrhld *ptr = first;

	if (!first)
		/* first module in the list */
		return new;

	/* find the last module in the list */
	while (ptr->next) {
		ptr = ptr->next;
	}

	ptr->next = new;

	return first;
}

/**
 * Delete modbus action and list
 */
void thrhld_delete(struct thrhld *thrh) {

	if (!thrh)
		return;

	thrhld_delete(thrh->next);

	free(thrh);

}

/**************************************************/
/* Handlers                                       */
/**************************************************/

/**
 * Update threshold state 
 */

int thrhld_state_upd(struct thrhld *thrh, float value) {
	int state = thrh->state;

	if (value >= thrh->max_thr)
		state = thrh->max_out;

	if (value <= thrh->min_thr)
		state = thrh->min_out;

	if (state == thrh->state)
		return 0;

	thrh->state = state;

	return 1;

}

int handler_rcv(EVENT_HANDLER_PAR) {
	struct thrhsmod_object* this = module_get_struct(handler->base);
	struct thrhld *thrh = (struct thrhld*) handler->objdata;
	struct uni_data *data = event->data;
	float value = uni_data_get_fvalue(data);

	PRINT_MVB(&this->base,"Received value : %f , prev state %d\n", value, thrh->state);

	if (thrhld_state_upd(thrh, value)) {
		module_base_send_event(module_event_create(thrh->event_type,
				uni_data_create_int(thrh->state), &event->time));
		PRINT_MVB(&this->base,"Send value  : %d , \n",thrh->state );
	}

	return 0;
}

/**
 * Read event handler
 */
struct module_event *modbus_eread(struct event_type *event) {
	struct thrhld *thrh = (struct thrhld *) event->objdata;
	struct thrhsmod_object* this = module_get_struct(event->base);

	return module_event_create(event, uni_data_create_int(thrh->state), NULL);
}

struct event_handler handlers[] = { { .name = "rcv", .function = handler_rcv },
		{ .name = "" } };

int start_threshold(XML_START_PAR) {
	//     struct thrhsmod_object* this = module_get_struct(ele->parent->data);
	struct modules *modules = (struct modules*) data;
	struct module_base* this = ele->parent->data;
	struct event_handler* handler = NULL;
	PRINT_MVB(this, "Opening threshold %s", get_attr_str_ptr(attr, "event") );
	handler = event_handler_get(this->event_handlers, "rcv");

	return module_base_subscribe_event(this, modules->list, get_attr_str_ptr(
			attr, "event"), FLAG_ALL, handler, attr);

}

struct xml_tag module_tags[] = { { "hysteresis", "module", start_threshold,
		NULL, NULL }, { "threshold", "module", start_threshold, NULL, NULL }, {
		"", "", NULL, NULL, NULL } };

struct event_handler *module_subscribe(struct module_base *module,
		struct module_base *source, struct event_type *event_type,
		const char **attr) {
	struct thrhsmod_object *this = module_get_struct(module);
	struct thrhld *new = thrhld_create(this, attr);
	struct event_handler *event_handler = NULL;
	const char *name = get_attr_str_ptr(attr, "name");
	const char *unit = get_attr_str_ptr(attr, "unit");
	char ename[64];
	if (strcmp(event_type->name, "fault") == 0)
		return NULL;

	sprintf(ename, "%s.%s", source->name, event_type->name);

	if (!name)
		name = event_type->name;

	if (!unit)
		unit = "";

	new->event_type = event_type_create(module, (void*) new, name, unit,
			get_attr_str_ptr(attr, "text"), event_type_get_flags(
					get_attr_str_ptr(attr, "flags"), module->flags));

	new->event_type->read = modbus_eread;

	event_handler = event_handler_create(event_type->name, handler_rcv,
			&this->base, (void*) new);
	this->base.event_handlers = event_handler_list_add(
			this->base.event_handlers, event_handler);
	this->base.event_types = event_type_add(this->base.event_types,
			new->event_type); // make it avaliable

	this->thrhs = thrhld_add(this->thrhs, new);
	PRINT_MVB(module, "subscribe %s %p", ename,event_handler );
	return event_handler;
}

struct module_type module_type = { .name = "threshold",
		.xml_tags = module_tags, .handlers = handlers,
		.type_struct_size = sizeof(struct thrhsmod_object), };

struct module_type *module_get_type() {
	return &module_type;
}

/**
 * @} 
 */

/**
 * @} 
 */
