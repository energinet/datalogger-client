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
#include "module_value.h"
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
 * @defgroup modules_levelalarm Module for handeling level alarms
 * @{
 *  
 * Module for handeling alarms.\n
 * <b>Type name: levelalarm </b>\n
 * <b>Attributes:</b>\n
 * - \b config: The configuration name @ref sgconf and @ref dinconf\n
 * .
 * <b> Tags: </b>
 */

enum levela_omode {
	LA_ONCHAN, /*!< Output event on change */
	LA_ALWAYS,
/*!< Output event on received event */
};

const char *throtyps[] = { "change", "always", NULL };

/**
 * Levelalarm object struct
 */
struct levelald {
	float state;
	int inittx;
	struct evalue *high_three;
	struct evalue *high_two;
	struct evalue *high_one;
	struct evalue *low_one;
	struct evalue *low_two;
	struct evalue *low_three;
	struct evalue *enabled;
	enum levela_omode omode;
	struct event_type *event_type;
	struct levelald *next;
};

/**
 * Levelalarm module object
 */
struct levelamod_object {
	/**
	 * Module base*/
	struct module_base base;
	/**
	 * Levelalarm list */
	struct levelald *levelas;
	/**
	 * Output mode */
	enum levela_omode omode;

};

struct levelamod_object* module_get_struct(struct module_base *module) {
	return (struct levelamod_object*) module;
}

/* // ToDo finde the bug in xmlpar compile */
/* float get_attr_float(const char **attr, const char *id, float rep_value) { */
/* 	int attr_no; */
/* 	float ret; */
/* 	char *format ; */

/* 	attr_no = get_attr_no(attr, id); */

/* 	if (attr_no < 0) { */
/* 		return rep_value; */
/* 	} */

/* 	if (sscanf(attr[attr_no], "%f", &ret) != 1) { */
/* 		printf("Attribute %s could not be red: %s (format %s)\n", id, */
/* 			   attr[attr_no], format); */
/* 		return rep_value; */
/* 	} */
/* 	printf("get_attr_float %f\n", ret); */

/* 	return ret; */
/* } */

/**
 * Create levelalarm
 */
struct levelald * levelald_create(struct levelamod_object* module,
		const char **attr) {

	struct levelald *new = malloc(sizeof(*new));
	const char *name = get_attr_str_ptr(attr, "name");
	const char *omode = get_attr_str_ptr(attr, "omode");
	assert(new);
	memset(new, 0, sizeof(*new));

	PRINT_MVB(&module->base, "111 %s", name);

	new->high_three = evalue_create(&module->base, name, "high_three",
			get_attr_str_ptr(attr, "high_three"), DEFAULT_FEVAL);
	new->high_two = evalue_create(&module->base, name, "high_two",
			get_attr_str_ptr(attr, "high_two"), DEFAULT_FEVAL);
	new->high_one = evalue_create(&module->base, name, "high_one",
			get_attr_str_ptr(attr, "high_one"), DEFAULT_FEVAL);
	new->low_one = evalue_create(&module->base, name, "low_one",
			get_attr_str_ptr(attr, "low_one"), DEFAULT_FEVAL);
	new->low_two = evalue_create(&module->base, name, "low_two",
			get_attr_str_ptr(attr, "low_two"), DEFAULT_FEVAL);
	new->low_three = evalue_create(&module->base, name, "low_three",
			get_attr_str_ptr(attr, "low_three"), DEFAULT_FEVAL);
	new->enabled = evalue_create(&module->base, name, "enabled", "1",
			DEFAULT_FEVAL);

	PRINT_MVB(&module->base, "113");
	new->state = get_attr_float(attr, "default", 0);
	PRINT_MVB(&module->base, "114");
	new->omode = modutil_get_listitem(omode, module->omode, throtyps);

	return new;

}

/**
 * Add modbus action to list
 */
struct levelald * levelald_add(struct levelald* first, struct levelald*new) {
	struct levelald *ptr = first;

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
void levelald_delete(struct levelald *levelah) {

	if (!levelah)
		return;

	levelald_delete(levelah->next);

	free(levelah);

}

/**************************************************/
/* Handlers                                       */
/**************************************************/

/**
 * Update levelalarm state
 */

int levelald_state_upd(struct levelald *levelah, float value) {

	if (value >= evalue_getf(levelah->high_three)) {
		return 3;
	} else if (value >= evalue_getf(levelah->high_two)) {
		return 2;
	} else if (value >= evalue_getf(levelah->high_one)) {
		return 1;
	} else if ((value < evalue_getf(levelah->high_one)) && (value
			> evalue_getf(levelah->low_one))) {
		return 0;
	} else if (value <= evalue_getf(levelah->low_three)) {
		return -3;
	} else if (value <= evalue_getf(levelah->low_two)) {
		return -2;
	} else if (value <= evalue_getf(levelah->low_one)) {
		return -1;
	}

	return 4;
}

int handler_rcv(EVENT_HANDLER_PAR) {
	struct levelamod_object* this = module_get_struct(handler->base);
	struct levelald *levelah = (struct levelald*) handler->objdata;
	struct uni_data *data = event->data;
	float value = uni_data_get_fvalue(data);
	int ivalue;

	PRINT_MVB(&this->base,"Received value : %f ,(high_three %f, high_two %f, high_one %f, low_one %f, low_two %f, low_three %f)\n", value,
			evalue_getf(levelah->high_three),evalue_getf(levelah->high_two),evalue_getf(levelah->high_one),evalue_getf(levelah->low_one),evalue_getf(levelah->low_two),evalue_getf(levelah->low_three));

	if ((int) evalue_getf(levelah->enabled)) {
		ivalue = levelald_state_upd(levelah, value);

		if ((ivalue != 4) || levelah->omode == LA_ALWAYS) {
			module_base_send_event(module_event_create(levelah->event_type,
					uni_data_create_float((float) ivalue), &event->time));
			PRINT_MVB(&this->base,"Send value  : %d , \n",ivalue );
		}
	}

	return 0;
}

/**
 * Read event handler
 */
struct module_event *modbus_eread(struct event_type *event) 
{
	struct levelald *levelah = (struct levelald *) event->objdata;
	
	return module_event_create(event, uni_data_create_int(levelah->state), NULL);
}

struct event_handler handlers[] = { { .name = "rcv", .function = handler_rcv },
		{ .name = "" } };

int start_levelalarm(XML_START_PAR) {
	//     struct levelamod_object* this = module_get_struct(ele->parent->data);
	struct modules *modules = (struct modules*) data;
	struct module_base* this = ele->parent->data;
	struct event_handler* handler = NULL;
	PRINT_MVB(this, "Opening levelalarm %s", get_attr_str_ptr(attr, "event") );
	handler = event_handler_get(this->event_handlers, "rcv");

	return module_base_subscribe_event(this, modules->list, get_attr_str_ptr(
			attr, "event"), FLAG_ALL, handler, attr);

}

struct xml_tag module_tags[] = { { "levels", "module", start_levelalarm, NULL,
		NULL }, { "levelalarm", "module", start_levelalarm, NULL, NULL }, { "",
		"", NULL, NULL, NULL } };

int module_init(struct module_base *base, const char **attr) {
	struct levelamod_object *this = module_get_struct(base);
	const char *omode = get_attr_str_ptr(attr, "omode");

	this->omode = modutil_get_listitem(omode, LA_ONCHAN, throtyps);

	return 0;
}

struct event_handler *module_subscribe(struct module_base *module,
		struct module_base *source, struct event_type *event_type,
		const char **attr) {
	struct levelamod_object *this = module_get_struct(module);
	struct levelald *new = levelald_create(this, attr);
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

	this->levelas = levelald_add(this->levelas, new);
	PRINT_MVB(module, "subscribe %s %p", ename,event_handler );
	return event_handler;
}

struct module_type module_type = { .name = "levelalarm",
		.xml_tags = module_tags, .handlers = handlers,
		.type_struct_size = sizeof(struct levelamod_object), };

struct module_type *module_get_type() {
	return &module_type;
}

/**
 * @} 
 */

/**
 * @} 
 */
