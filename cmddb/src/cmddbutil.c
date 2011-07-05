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
#include <sqlite3.h>
#include <stdlib.h>
#include <getopt.h>

#include "cmddb.h"

const char *HelpText = "cmddbutil: util for command database... \n"
	" -f <db_file>    : Path to the command database file\n"
	" -i <cid>        : Command id\n"
	" -a <name>       : Add commans\n"
	" -v <param>      : Parameter/value Debug level \n"
	" -c <name>       : Get current value\n"
	" -e <name>       : Execute command \n"
	" -r <cid>        : Mark for removeal\n"
	" -R <cid>        : Remove commans (hart)\n"
	" -p              : Print commands in database \n"
	" -l              : List current values \n"
	" -C              : Clean non-sent history\n"
	" -d <level>      : Set debug level \n"
	" -h              : Help (this text)\n"
	"Copyright (C) LIAB ApS - CRA, January 2010\n"
	"";

int test_handler(CMD_HND_PARAM) {

	return CMDSTA_EXECUTED;
}

int main(int narg, char *argp[]) {
	int optc;
	int debug_level = 0;

	int cid = 0;
	int do_print = 0;
	int do_list = 0;
	int do_nonsent_clean = 0;
	time_t now = time(NULL);
	char *cmd_add = NULL;
	char *cmd_val = NULL;
	char *cmd_rem = NULL;
	char *cmd_hardrem = NULL;
	char *get_val = NULL;
	char *db_path = NULL;

	struct cmddb_desc *exe_list = NULL;

	while ((optc = getopt(narg, argp, "c:f:i:a:v:e:r:R:plCd::h")) > 0) {
		switch (optc) {
		case 'c':
			get_val = strdup(optarg);
			break;
		case 'f':
			db_path = strdup(optarg);
			break;
		case 'i':
			cid = atoi(optarg);
		case 'a':
			cmd_add = strdup(optarg);
			break;
		case 'v':
			cmd_val = strdup(optarg);
			break;
		case 'e':
			exe_list = cmddb_desc_add(exe_list, cmddb_desc_create(optarg, NULL,
					NULL, 0));
			break;
		case 'r':
			cmd_rem = strdup(optarg);
			break;
		case 'R':
			cmd_hardrem = strdup(optarg);
			break;
		case 'p':
			do_print = 1;
			break;
		case 'l':
			do_list = 1;
			break;
		case 'C':
			do_nonsent_clean = 1;
			break;
		case 'd':
			if (optarg)
				debug_level = atoi(optarg);
			else
				debug_level = 1;
			break;
		default:
			fprintf(stderr, "%s", HelpText);
			exit(0);
			break;
		}
	}

	cmddb_db_open(db_path, debug_level);

	if (do_print)
		cmddb_db_print();

	if (cmd_rem) {
		cmddb_mark_remove(atoi(cmd_rem));
	}

	if (cmd_hardrem) {
		cmddb_remove(atoi(cmd_hardrem));
	}

	if (get_val) {
		char buffer[512];
		cmddb_value(get_val, buffer, sizeof(buffer), now);
		printf("getval: %s = %s\n", get_val, buffer);
	}

	if (cmd_add) {
		cmddb_insert(cid, cmd_add, cmd_val, time(NULL), 0, now);
	}

	if (exe_list) {
		int retval = cmddb_exec_list(exe_list, NULL, now);
		printf("exe list returned %d\n", retval);
	}

	if (do_nonsent_clean)
		cmddb_notsent_clean();

	cmddb_db_close();

	cmddb_desc_delete(exe_list);
	free(cmd_add);
	free(cmd_val);
	free(cmd_rem);
	free(cmd_hardrem);
	free(get_val);
	free(db_path);

	return 0;

}

