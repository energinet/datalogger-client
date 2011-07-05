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

#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

#include "asocket.h"
#include "asocket_server.h"
#include "asocket_trx.h"
#include "asocket_client.h"

const char *HelpText = " -h           : Help (this text)\n"
	"Copyright (C) LIAB ApS - CRA, May 2010\n"
	"";

int asocket_msg_test(void) {
	int retval = 0;
	int len;
	char cmdbuf[1024];
	char errbuf[32];
	struct skcmd* cmd = asocket_cmd_create("BoxInfo");

	printf("Create cmd BoxInfo %p\n", cmd);

	assert(cmd);

	retval = asocket_cmd_param_add(cmd, "box,-id");
	printf("Add box-id (%d)\n", retval);

	retval = asocket_cmd_param_add(cmd, "\"box-type");
	printf("Add box-type (%d)\n", retval);

	retval = asocket_cmd_param_add(cmd, "box-version");
	printf("Add box-version (%d)\n", retval);

	retval = asocket_cmd_write(cmd, cmdbuf, 1024);
	printf("Write cmd: %d/%d : %s\n", retval, strlen(cmdbuf), cmdbuf);
	printf("param cnt: %d\n", asocket_cmd_param_count(cmd));
	len = retval;

	asocket_cmd_delete(cmd);

	cmd = asocket_cmd_create("...");

	retval = asocket_cmd_read(cmd, cmdbuf, len, errbuf);
	printf("Read cmd: %d\n", retval);

	retval = asocket_cmd_write(cmd, cmdbuf, 1024);
	printf("Result: %d/%d : %s\n", retval, strlen(cmdbuf), cmdbuf);
	printf("param cnt: %d\n", asocket_cmd_param_count(cmd));

	asocket_cmd_delete(cmd);

	return 0;
}

struct asocket_cmd socket_cmd_list[] = { { "status", asocket_cmd_stub,
		"Help text 1" }, { "hup", asocket_cmd_stub, "jkjkj" }, { "exit", NULL,
		"Close the connection" }, { NULL, NULL } };

int asocket_srv_test(int port) {
	struct socket_data param;
	int i;

	param.appdata = NULL;
	param.skaddr = asocket_addr_create_in(NULL, port);
	param.cmds = socket_cmd_list;

	asckt_srv_start(&param);

	scanf("%d", &i);

	asckt_srv_close(&param);

	return 0;
}

static char *relay[] = { "relay-X1", "relay-X2", "relay-X3", "relay-X4", NULL };

void set_relay(int index, int state) {
	FILE *fp;
	char buf[128];

	snprintf(buf, sizeof(buf), "/sys/class/leds/%s/brightness", relay[index]);
	printf("set relay: %s\n", buf);
	if ((fp = fopen(buf, "w")) != NULL) {
		fprintf(fp, "%d", state ? 1 : 0);
		fflush(fp);
		fclose(fp);
	}
}

struct vpp_box {
	char *id;
	char *type;
	char *verion;
	int output[2];
	int meter[1];
};

int vpp_cmd_boxinfo(ASOCKET_FUNC_PARAM) {
	struct vpp_box *vpp_box = (struct vpp_box *) sk_con->appdata;

	asocket_con_snd_vp(sk_con, "BoxInfo", 3, vpp_box->id, vpp_box->type,
			vpp_box->verion);

	return 0;
}

int vpp_cmd_ioset(ASOCKET_FUNC_PARAM) {
	struct vpp_box *vpp_box = (struct vpp_box *) sk_con->appdata;
	char *name_str = asocket_cmd_param_get(rx_msg, 0);
	char *value_str = asocket_cmd_param_get(rx_msg, 1);
	int number;

	printf("vpp_cmd_ioset\n");

	if (!name_str) {
		asocket_con_snd_nack(sk_con, "to few arguments");
		return 0;
	}

	number = atoi(name_str);

	if (number < 1 || number > 2) {
		asocket_con_snd_nack(sk_con, "unknown io");
		return 0;
	}

	if (value_str) {
		if (strcmp(value_str, "on") == 0) {
			vpp_box->output[number - 1] = 1;
		} else if (strcmp(value_str, "off") == 0) {
			vpp_box->output[number - 1] = 0;
		} else {
			asocket_con_snd_nack(sk_con, "unknown value");
			return 0;
		}
		set_relay(number - 1, vpp_box->output[number - 1]);

	}

	asocket_con_snd_vp(sk_con, "IoUpdate", 2, name_str, (vpp_box->output[number
			- 1]) ? "on" : "off");

	return 0;
}

int vpp_cmd_statusget(ASOCKET_FUNC_PARAM) {
	asocket_con_snd_vp(sk_con, "Status", 2, "00:15:8C:00:18:26", "999999");

	return 0;
}

int vpp_cmd_readingget(ASOCKET_FUNC_PARAM) {
	struct vpp_box *vpp_box = (struct vpp_box *) sk_con->appdata;
	char *name_str = asocket_cmd_param_get(rx_msg, 0);
	int number;

	if (!name_str) {
		asocket_con_snd_nack(sk_con, "to few arguments");
		return 0;
	}

	number = atoi(name_str);

	if (number < 1 || number > 1) {
		asocket_con_snd_nack(sk_con, "unknown meter");
		return 0;
	}

	asocket_con_snd_vp(sk_con, "ReadingGet", 2, name_str, "999999");

	return 0;
}

int vpp_client(const char *ip, int port) {
	struct sockaddr * skaddr = asocket_addr_create_in(ip, port);
	struct asocket_con* sk_con = NULL;

	struct vpp_box vpp_box = { "00:15:8C:00:18:26", "demo", "0.01", { 0, 0 }, {
			0 } };

	struct asocket_cmd vpp_cmd_list[] = {
			{ "BoxInfoGet", vpp_cmd_boxinfo, NULL }, { "IoSet", vpp_cmd_ioset,
					NULL }, { "StatusGet", vpp_cmd_statusget, NULL }, {
					"ReadingGet", vpp_cmd_readingget, NULL }, { "Close", NULL,
					NULL }, { NULL, NULL } };

	sk_con = asocket_clt_connect(skaddr);

	free(skaddr);

	if (!sk_con) {
		fprintf(stderr, "Socket connect failed\n");
		return -errno;
	}

	/* TX box info */
	asocket_con_snd_vp(sk_con, "BoxInfo", 3, vpp_box.id, vpp_box.type,
			vpp_box.verion);

	sk_con->cmds = vpp_cmd_list;
	sk_con->appdata = &vpp_box;

	asocket_con_rcvr(sk_con);

	return asocket_clt_disconnect(sk_con);

}

int main(int narg, char *argp[]) {
	int optc;
	int retval = 0;
	int port = 7413;

	while ((optc = getopt(narg, argp, "hs:mv:p:")) > 0) {
		switch (optc) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'v':
			return vpp_client(optarg, port);
		case 's':
			return asocket_srv_test(atoi(optarg));
		case 'm':
			return asocket_msg_test();
		case 'h':
		default:
			fprintf(stderr, "%s", HelpText);
			exit(0);
			break;
		}
	}

	return retval;

}
