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
#include <unistd.h>
#include <assert.h>
struct module_object {
	struct module_base base;

	float Cvand;
	float q;
	float Tr;
	float Tf;
	struct event_type *event_type;
};

struct module_object* module_get_struct(struct module_base *module) {
	return (struct module_object*) module;
}

void module_send_event(struct module_object *module,
		struct event_type *event_type, struct timeval *time,
		unsigned long diff, float value) {
	struct uni_data data;

	memset(&data, 0, sizeof(data));

	data.type = DATA_FLOAT;
	data.value = (float) value;
	data.mtime = diff;
	data.data = &value;

	module_base_send_event(module_event_create(event_type, &data, time));
}

int handler_flow(EVENT_HANDLER_PAR) {
	struct module_object* this = module_get_struct(handler->base);
	struct uni_data *data = event->data;

	this->q = uni_data_get_fvalue(data);

}

int handler_tr(EVENT_HANDLER_PAR) {
	struct module_object* this = module_get_struct(handler->base);
	struct uni_data *data = event->data;

	this->Tr = uni_data_get_fvalue(data);

	{
		// Q [kJ/sek] = d [kg/liter] x q [liter/min] x Cvand [KJ/(kg x °C)] x (Tf - Tr) [°C] x (1/60) min/sek
		float Q = ((this->q / 60/*l/s*/) * this->Cvand * (this->Tf - this->Tr))
				* 1000;
		PRINT_MVB(&this->base, "Q = %f\n", Q);
		module_send_event(this, this->event_type, &event->time, 1, Q);
	}

}

int handler_tf(EVENT_HANDLER_PAR) {
	struct module_object* this = module_get_struct(handler->base);
	struct uni_data *data = event->data;

	this->Tf = uni_data_get_fvalue(data);

}

struct module_event *power_eread(struct event_type *event) {
	struct module_object* this = module_get_struct(event->base);
	float Q = ((this->q / 60/*l/s*/) * this->Cvand * (this->Tf - this->Tr))
			* 1000;

	return module_event_create(event, uni_data_create_float(Q), NULL);
}

struct event_handler handlers[] = {
		{ .name = "flow", .function = handler_flow }, { .name = "Tr",
				.function = handler_tr },
		{ .name = "Tf", .function = handler_tf }, { .name = "" } };
struct event_type events[] = { { .name = "change" }, { .name = "fault" }, {
		.name = "" } };

int start_listen(XML_START_PAR) {
	struct modules *modules = (struct modules*) data;
	struct module_object* this = module_get_struct(ele->parent->data);
	struct event_handler* handler = NULL;

	handler = event_handler_get(this->base.event_handlers, "flow");
	assert(handler);
	module_base_subscribe_event(&this->base, modules->list, get_attr_str_ptr(
			attr, "flow"), FLAG_ALL, handler, attr);

	handler = event_handler_get(this->base.event_handlers, "Tr");
	assert(handler);
	module_base_subscribe_event(&this->base, modules->list, get_attr_str_ptr(
			attr, "Tr"), FLAG_ALL, handler, attr);

	handler = event_handler_get(this->base.event_handlers, "Tf");
	assert(handler);
	module_base_subscribe_event(&this->base, modules->list, get_attr_str_ptr(
			attr, "Tf"), FLAG_ALL, handler, attr);

	this->Cvand = get_attr_int(attr, "Cvand", 4.182);

	this->event_type = event_type_create_attr(&this->base, NULL, attr);
	this->event_type->read = power_eread;

	this->base.event_types = event_type_add(this->base.event_types,
			this->event_type);

	return 0;
}

struct xml_tag module_tags[] = {
		{ "power", "module", start_listen, NULL, NULL }, { "", "", NULL, NULL,
				NULL } };

struct module_type module_type = { .name = "waterpower",
		.mtype = MTYPE_CONTROL, .subtype = SUBTYPE_FLOW,
		.xml_tags = module_tags, .handlers = handlers,
		.type_struct_size = sizeof(struct module_object), };

struct module_type *module_get_type() {
	return &module_type;
}
