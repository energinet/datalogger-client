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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/time.h>

#include <asocket_trx.h>
#include <asocket_server.h>

#define DEFAULT_PORT 6523

struct module_object;

struct module_object* module_get_struct(struct module_base *module) {
	return (struct module_object*) module;
}

/**
 * Socket functions 
 */

struct sconn {
	struct module_object* module;
	struct module_event* event_list;
	struct module_event* event_next;
};

struct module_object {
	struct module_base base;
	struct callback_list* readlist;
	struct sconn sconn;
	struct socket_data param;
};

int socket_hndlr_reads_upd(ASOCKET_FUNC_PARAM) {
	struct sconn *sconn = (struct sconn*) sk_con->appdata;
	struct module_object* this = sconn->module;
	struct module_base* base = &this->base;
	struct skcmd *tx_msg = asocket_cmd_create("Reads");
	unsigned long mask = event_type_get_flags(asocket_cmd_param_get(rx_msg, 0),
			FLAG_SHOW );

	/* delete prev events */
	if (sconn->event_list)
		module_event_delete(sconn->event_list);

	sconn->event_list = module_base_read_all(base->first,
			asocket_cmd_param_get(rx_msg, 1), mask);
	sconn->event_next = sconn->event_list;

	asocket_con_snd(sk_con, tx_msg);
	asocket_cmd_delete(tx_msg);

	return 0;

}

int socket_hndlr_read_get(ASOCKET_FUNC_PARAM) {
	struct sconn *sconn = (struct sconn*) sk_con->appdata;
	struct skcmd *tx_msg = asocket_cmd_create("Read");

	if (sconn->event_next) {
		struct module_event* event = sconn->event_next;
		struct event_type *type = event->type;
		char buf[512];
		snprintf(buf, sizeof(buf), "%s.%s", event->source->name,
				event->type->name);
		asocket_cmd_param_add(tx_msg, buf);
		asocket_cmd_param_add(tx_msg, event->type->hname);
		uni_data_get_txt_fvalue(event->data, buf, sizeof(buf), type->flunit);
		asocket_cmd_param_add(tx_msg, buf);
		if (!type->flunit)
			asocket_cmd_param_add(tx_msg, type->unit);
		else
			asocket_cmd_param_add(tx_msg, type->flunit->lunit);

		sconn->event_next = sconn->event_next->next;
	}

	asocket_con_snd(sk_con, tx_msg);
	asocket_cmd_delete(tx_msg);

	return 0;

}

struct socket_cmd socket_cmd_list[] = { { "ReadsUpdate",
		socket_hndlr_reads_upd,
		"Update read values param1: flags, param2: name " }, { "ReadGet",
		socket_hndlr_read_get, "Get read value" }, { "exit", NULL,
		"Close the connection" }, { "Exit", NULL, "Close the connection" }, {
		NULL, NULL } };

struct xml_tag module_tags[] = { { "", "", NULL, NULL, NULL } };

struct event_handler handlers[] = { { .name = "" } };

int module_init(struct module_base *module, const char **attr) {
	struct module_object* this = module_get_struct(module);

	this->readlist = NULL;
}

int module_server_start(struct socket_data *param, struct sconn *sconn) {
	param->appdata = (void*) sconn;
	param->skaddr = asocket_addr_create_in(NULL, DEFAULT_PORT);
	param->cmds = socket_cmd_list;
	return asckt_srv_start(param);

}

int module_start(struct module_base *module) {
	struct module_object* this = module_get_struct(module);
	struct module_base *ptr = module->first;
	int i = 0, n = 0;

	while (ptr) {
		struct event_handler *handler = event_handler_get(ptr->event_handlers,
				"read");
		if (handler) {
			i++;
			PRINT_MVB(module, "Added %s", ptr->name);
			this->readlist = callback_list_add(this->readlist,
					callback_list_create(handler));
		}
		n++;
		ptr = ptr->next;
	}

	PRINT_MVB(module, "found %d/%d %p", i, n, this->readlist);

	this->sconn.module = this;

	return module_server_start(&this->param, &this->sconn);
}

void module_deinit(struct module_base *module) {
	struct module_object* this = module_get_struct(module);

	asckt_srv_close(&this->param);

	callback_list_delete(this->readlist);
	return;
}

struct module_type module_type = { .name = "socket", .mtype = MTYPE_OUTPUT,
		.subtype = SUBTYPE_LEVEL, .xml_tags = NULL, .handlers = handlers,
		.type_struct_size = sizeof(struct module_object), };

struct module_type *module_get_type() {
	return &module_type;
}
