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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include "type_loader.h"
#include "module_type.h"
#include "module_base.h"

#define DEFAULT_PIDFILE   "/var/run/contdaem.pid"
#define DEFAULT_CONFFILE  "/etc/contdaem.conf"

const char *HelpText =
"contdaem: Daemon for logging data and controling peripherals ... \n"
" -m <conf>    : Set module configuration file (default "DEFAULT_CONFFILE")\n"
" -P <dir>     : Set plugin directory (default "PLUGINDIR")\n"
" -p <dir>     : Set extra plugin directory\n"
" -h           : Help (this text)\n"
"Copyright (C) LIAB ApS - CRA, February 2010\n"
"";

int pidfile_pidwrite(const char *pidfile) {
	FILE *fp;

	if ((fp = fopen(pidfile, "w")) == NULL) {
		fprintf(stderr, "Error: could not open pidfile for writing: %s\n",
				strerror(errno));
		return errno;
	}

	fprintf(fp, "%ld\n", (long) getpid());

	fclose(fp);

	return 0;
}

int pidfile_create(const char *pidfile) {
	int ret;
	struct stat s;
	FILE *fp;
	int filepid;

	ret = stat(pidfile, &s);

	if (ret == 0) {
		/* file exits: open file */
		if ((fp = fopen(pidfile, "r")) == NULL) {
			fprintf(stderr, "Error: could not open pidfile: %s\n", strerror(
					errno));
			return -errno;
		}

		/* read pid */
		ret = fscanf(fp, "%d[^\n]", &filepid);
		fclose(fp);

		if (ret != 1) {
			fprintf(stderr, "Error: reading pidfile: %s\n", strerror(errno));
			return -errno;
		}

		printf("pid in file is  %d\n", filepid);

		if (kill(filepid, 0) == 0) {
			printf("pid is running... \n");
			return 0;
		}
	}

	pidfile_pidwrite(pidfile);

	return 1;

}

int pidfile_close(const char *path) {
	if (unlink(path) != 0)
		return -1;

	return 0;

}

int run = 1;
int hupped = 0;

void signal_handler(int sig) {

	switch (sig) {
	case SIGHUP:
		hupped = 1;
		break;
	case SIGINT:
		hupped = 1;
		if (run)
			run = 0;
		else
			exit(0);
		break;
	default:
		break;
	}

}

int main(int narg, char *argp[]) {
	int debug_level = 0;
	int optc;
	struct modules modules;
	char *moduledir = PLUGINDIR;
	char *configfile = DEFAULT_CONFFILE;
	int run_forground = 0;
	memset(&modules, 0, sizeof(modules));

	signal(SIGINT, signal_handler);
	signal(SIGHUP, signal_handler);

	while ((optc = getopt(narg, argp, "hp:P:m:d:l:f")) > 0) {
		switch (optc) {
		/* Help */
		case 'h':
			fprintf(stderr, "%s", HelpText);
			exit(0);
			break;
		case 'p': /* load extra module path */
			if (module_type_load_all(optarg, &modules.types) < 0)
				return -1;
			break;
		case 'P':
			moduledir = strdup(optarg);
			break;
		case 'm':
			configfile = strdup(optarg);
			break;
		case 'd':
			debug_level = atoi(optarg);
			modules.verbose = debug_level;
			break;
		case 'l':
			modules.loadlist = load_list_add(modules.loadlist, optarg);
			break;
		case 'f':
			run_forground = 1;
			break;
		default:
			fprintf(stderr, "%s", HelpText);
			exit(0);
			break;
		}
	}

	if (!run_forground) {
		fprintf(stderr, "daemonizing config file: %s\n", configfile);
		if (daemon(1, 0)) {
			PRINT_ERR("daemon returned error: %s", strerror(errno));
			exit(1);
		}
	}

	pidfile_create(DEFAULT_PIDFILE);

	/* load modules */
	if (module_type_load_all(moduledir, &modules.types) < 0) {
		pidfile_close(DEFAULT_PIDFILE);
		return -1;
	}

	printf("Types loaded:\n");
	module_type_list_print(modules.types);

	/* load config */
	if (!configfile) {
		PRINT_ERR("no config file");
		pidfile_close(DEFAULT_PIDFILE);
		return -1;
	}

	if (parse_xml_file(configfile, base_xml_tags, &modules) < 0)
		return 1;

	while (run)
		sleep(1);

	printf("deleting modules\n");

	module_base_delete(modules.list);
	module_type_delete(modules.types);

	printf("contdaem ended\n");

	return 0;

}
