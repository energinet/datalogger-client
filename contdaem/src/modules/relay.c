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
#include <assert.h>

/**
 * @ingroup modules 
 * @{
 */

/**
 * @defgroup modules_relay Module for setting relays
 * @{
 */

struct module_object {
	struct module_base base;
	char *device;
};

struct module_object* module_get_struct(struct module_base *module) {
	return (struct module_object*) module;
}

int relay_set_output(struct module_object *module, int value) {
	FILE *file = NULL;

	if (!module->device) {
		PRINT_ERR("no device");
		return -EFAULT;
	}

	file = fopen(module->device, "w");

	if (!file) {
		PRINT_ERR("error opening %s: %s", module->device, strerror(errno));
		return -errno;
	}

	fprintf(file, "%d", value);

	fclose(file);

	return 0;

}

int event_handler_set_state(EVENT_HANDLER_PAR) {
	struct module_object* this = module_get_struct(handler->base);
	struct uni_data *data = event->data;

	PRINT_MVB(&this->base,"Module type %s received event from %s\n", this->base.name, event->source->name);

	if (data->value)
		relay_set_output(this, 1);
	else
		relay_set_output(this, 0);

	return 0;
}

struct event_handler handlers[] = { { .name = "state",
		.function = event_handler_set_state }, { .name = "" } };
struct event_type events[] = { { .name = "change" }, { .name = "fault" }, {
		.name = "" } };

int start_device(XML_START_PAR) {
	struct module_object* this = module_get_struct(ele->parent->data);
	const char *text = get_attr_str_ptr(attr, "device");
	const char *listen = get_attr_str_ptr(attr, "listen");

	if (!text) {
		PRINT_ERR("no text");
		return -EFAULT;
	}

	this->device = strdup(text);

	return 0;
}

int start_listen(XML_START_PAR) {
	struct modules *modules = (struct modules*) data;
	struct module_base* this = ele->parent->data;
	struct event_handler* handler = NULL;

	handler = event_handler_get(this->event_handlers, "state");

	assert(handler);
	fprintf(stderr, "listen to %s\n", get_attr_str_ptr(attr, "event"));

	return module_base_subscribe_event(this, modules->list, get_attr_str_ptr(
			attr, "event"), FLAG_ALL, handler, attr);
}

struct xml_tag module_tags[] = {
		{ "listen", "module", start_listen, NULL, NULL }, { "output", "module",
				start_device, NULL, NULL }, { "", "", NULL, NULL, NULL } };

int module_init(struct module_base *module, const char **attr) {
	struct module_object* this = module_get_struct(module);

	this->device = NULL;

	return 0;
}

struct module_type
		module_type = { .name = "relay", .mtype = MTYPE_OUTPUT,
				.subtype = SUBTYPE_EVENT, .xml_tags = module_tags,
				.handlers = handlers,
				.type_struct_size = sizeof(struct module_object), };

struct module_type *module_get_type() {
	return &module_type;
}

/**
 * @} 
 */

/**
 * @} 
 */
