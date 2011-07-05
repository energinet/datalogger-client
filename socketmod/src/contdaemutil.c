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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <asocket_trx.h>
#include <asocket_client.h>

#define DEFAULT_PORT 6523

const char *HelpText = "contdaemutil: util for contdaem... \n"
	" -l              : List event types and values\n"
	"Copyright (C) LIAB ApS - CRA, June 2010\n"
	"";

int live_client_write(int i);

int main(int narg, char *argp[]) {
	int optc;
	int print_events = 0;

	while ((optc = getopt(narg, argp, "lhs:")) > 0) {
		switch (optc) {
		case 's':
			live_client_write(atoi(optarg));
			break;
		case 'l': // list db events
			print_events = 1;
			break;
		case 'h': // Help
		default:
			fprintf(stderr, "%s", HelpText);
			exit(0);
			break;
		}
	}

	if (print_events)
		live_client_print_line();

	return 0;

}

struct asocket_con* sk_con = NULL;

int live_client_connect(void) {
	struct sockaddr * skaddr =
			asocket_addr_create_in("127.0.0.1", DEFAULT_PORT);

	sk_con = asocket_clt_connect(skaddr);

	free(skaddr);

	if (!sk_con) {
		fprintf(stderr, "Socket connect failed\n");
		return -errno;
	}

	return 0;
}

int live_client_disconnect(void) {
	return asocket_clt_disconnect(sk_con);
}

int live_client_upd() {
	struct skcmd* tx_msg = asocket_cmd_create("ReadsUpdate");
	struct skcmd* rx_msg = NULL;

	asocket_con_trancive(sk_con, tx_msg, &rx_msg);

	asocket_cmd_delete(tx_msg);

	if (!rx_msg)
		return -EFAULT;

	asocket_cmd_delete(rx_msg);

	return 0;
}

int live_client_print_line() {
	struct skcmd* tx_msg = asocket_cmd_create("ReadGet");
	struct skcmd* rx_msg = NULL;

	live_client_connect();
	live_client_upd();

	while (1) {
		asocket_con_trancive(sk_con, tx_msg, &rx_msg);
		if (!rx_msg)
			break;

		if (!rx_msg->param)
			break;

		printf(" %-20s   %15s   %s\n", asocket_cmd_param_get(rx_msg, 1),
				asocket_cmd_param_get(rx_msg, 2), asocket_cmd_param_get(rx_msg,
						3));

		asocket_cmd_delete(rx_msg);

	}

	asocket_cmd_delete(tx_msg);

	live_client_disconnect();
	return 0;
}

int live_client_write(int val) {
	struct skcmd* tx_msg = asocket_cmd_create("Write");
	struct skcmd* rx_msg = NULL;
	char tmpbuf[256];
	live_client_connect();

	asocket_cmd_param_add(tx_msg, "relay00.but2");
	sprintf(tmpbuf, "%d", val);
	asocket_cmd_param_add(tx_msg, tmpbuf);

	asocket_con_trancive(sk_con, tx_msg, &rx_msg);

	if (rx_msg) {
		printf("responce\n");
		asocket_cmd_delete(rx_msg);
	}

	asocket_cmd_delete(tx_msg);
	live_client_disconnect();

	return 0;
}
