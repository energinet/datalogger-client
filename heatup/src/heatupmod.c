/*
 * Copyright (C) 2015 LIAB ApS <info@liab.dk>
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
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/*Contdaem includes*/
#include <module_base.h>
#include <module_value.h>

/*Modbus IPC*/
#include <sys/msg.h>
#include <msgpipe.h> 

#define DEFAULT_INTERVAL 30
#define MSG_LEN 200
#define DVI_ADDR 0x42
#define TIMEOUT 1000

int prep_msg(char *ptr, int max, int addr, enum eRegTypes type, enum eActionTypes action, int reg, int mult, int value, enum eValueTypes val_type);

int write_cmd(int mq_id,  int addr, enum eRegTypes type, int reg_no, int mult, int value, int dbglev);

struct module_object {
	struct module_base base;
	int check_interval;
	int sgready;
	int old_curve;
	int current_curve;
	int message_queue;
	pthread_mutex_t message_lock[1];
};

static int read_write_holding(struct module_object *this, int reg, int write_val) {
	struct sIPCMsg msg[1];
	char *tmp;
	int timeout;
	int ret = -1;
	int val = 0;

	if (write_val > 255)
		return -1;

	if (write_val >= 0)
		val = write_val + 0x100; //Special DVI implementation

	prep_msg(msg->text, sizeof(msg->text), DVI_ADDR, MB_TYPE_HOLDING, MA_TYPE_WRITE, reg, 0, val, MV_TYPE_SHORT);
	printf("message: %s\n", msg->text);

	timeout = TIMEOUT; //~100 ms
	msg->type = 1;
	pthread_mutex_lock(this->message_lock);
	while(msgsnd(this->message_queue, msg, sizeof(msg->text), IPC_NOWAIT) != 0 && timeout--) {
		usleep(1000);
	}

	if (timeout == 0)
		goto error;

	timeout = TIMEOUT;
	while(msgrcv(this->message_queue, msg, sizeof(msg->text), 2, IPC_NOWAIT) < 0 && timeout--) {
		usleep(1000);
	}

	if (timeout == 0)
		goto error;

	printf("received: %s\n", msg->text);

	if (strstr(msg->text,"ERROR"))
		goto error;

	tmp = strstr(msg->text, "RESULT:");
	if (sscanf(tmp, "RESULT:%d", &ret) != 1)
		ret = -1;

	ret &= 0xff;

error:
	printf("Returning exit state %d\n", ret);
	pthread_mutex_unlock(this->message_lock);
	return ret;
}

static int read_curve (struct module_object *this) {
	return read_write_holding(this, 2, -1);
}

static int write_curve (struct module_object *this, int value) {
	return read_write_holding(this, 2, value);
}

static int setheating(struct module_object *this, char reg1, char reg10) {
	int retries = 2*5;

	while (read_write_holding(this, 1, reg1) < 0 && retries--);
	while (read_write_holding(this, 10, reg10) < 0 && retries--);

	if (retries)
		return 0;
	else
		return -1;
}

int sgready_change (EVENT_HANDLER_PAR) {
	struct module_object *this;
	int sgready;
	int retries;
	int curve;
	int res;

	this = (struct module_object *) handler->base;
	sgready = (int) uni_data_get_value(event->data);

	if (sgready == this->sgready)
		return 0;

	flush_msg_ipc(this->message_queue);

	//Read the current curve variable if changing from <=1 to >1
	if (sgready > 1 && this->sgready <= 1) {
		retries = 5;
		while ( (this->old_curve = read_curve(this)) < 0 && retries--);

		if (this->old_curve < 0)
			return -1;
	}

	if (sgready > 1) {
		curve = this->old_curve;

		if (sgready == 2)
			curve += 5;
		if (sgready == 3)
			curve = 20;

		if (curve > 20)
			curve = 20;

		retries = 5;
		while ( (res = write_curve(this,curve)) < 0 && retries--);

		if (res < 0)
			return -1;

		this->current_curve = curve;
	}

	//Reset curve if going back to normal or stopped
	if (this->sgready > 1 && sgready <= 1) {

		//Test if the user has manipulated the curve
		retries = 5;
		while ( (curve = read_curve(this)) < 0 && retries--);

		if (curve < 0)
			return -1;

		//If the curve is as we wrote it, put it back
		if (curve == this->current_curve) {
			curve = this->old_curve;
			retries = 5;
			while ( (res = write_curve(this,curve)) < 0 && retries--);

			if (res < 0)
				return -1;
		}
		this->current_curve = curve;
	}

	res = 0;
	switch (sgready) {
		case 0: //Normal
		case 2: //Forced 1
			res = setheating(this,1,1);
			printf("Setting heating 1,1\n");
			break;
		case 1: //Stopped
			res = setheating(this,0,0);
			printf("Setting heating 0,0\n");
			break;
		case 3: //Forced 2
			res = setheating(this,2,1);
			printf("Setting heating 2,1\n");
			break;
	}

	printf("Old curve was: %d\n", this->old_curve);
	printf("New curve is: %d\n", this->current_curve);

	this->sgready = sgready;
	printf("SGREADY CHANGED TO %d <-------------\n", sgready);
	return res;
}

int *module_init (struct module_base *base, const char **attr) {
	(void) attr;
	struct module_object *this = (struct module_object *) base;

	this->sgready = -1;

	this->message_queue = connect_ipc();
	flush_msg_ipc(this->message_queue);

	pthread_mutex_init(this->message_lock, NULL);

	return NULL;
}

void module_deinit( struct module_base *base) {
	struct module_object *this = (struct module_object *) base;

	destroy_ipc(this->message_queue);
	pthread_mutex_destroy(this->message_lock);
}

int start_sgready (XML_START_PAR) {
	(void) el;
    struct modules *modules = (struct modules*)data;
    struct module_base* base = ele->parent->data;
	struct event_handler *handler;
	const char *sgready;

	sgready = get_attr_str_ptr(attr, "var");
	if (!sgready)
		return -1;

	handler = event_handler_create("sgready", sgready_change ,base, NULL);
    base->event_handlers = event_handler_list_add(base->event_handlers, handler);

	return module_base_subscribe_event(base, modules->list, sgready, FLAG_ALL,  handler, attr);
}

void *module_loop ( void *param ) {
	printf("HEATUP LOOP\n");
	struct module_object *this = (struct module_object *) param;
	struct module_base *base = &this->base;
	while (base->run) {sleep(1);}

	return NULL;
}

struct xml_tag module_tags[] = {
    { "sgready" , "module" , start_sgready, NULL, NULL},
    { "" , "" , NULL, NULL, NULL }
};

struct module_type module_type = {
    .name       = "heatup",
    .xml_tags   = module_tags,
    .type_struct_size = sizeof(struct module_object),
};


struct module_type *module_get_type()
{
    return &module_type;
}
