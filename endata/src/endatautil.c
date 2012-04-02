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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <getopt.h>
#include <endata.h>

const char *HelpText = "endatautil: util for energinet data service lib... \n"
" -i <inst_id>    : Set installation ID\n"
" -D <offset>     : Set day offset\n"
" -l <language>   : Set language\n"
" -p <type>       : Set requested type\n"
" -d <level>      : Set debug level \n"
" -h              : Help (this text)\n"
"Copyright (C) LIAB ApS - CRA, June 2011\n"
"";

int main(int narg, char *argp[]) {
	int optc;
	int deblev = 0;
	int inst_id = 0;
	int day;

	char *address = NULL;
	enum endata_type select = ENDATA_ALL;
	enum endata_lang language = ENDATA_LANG_UK; 
	
	while ((optc = getopt(narg, argp, "a:i:D:l:p:d::h"))
			> 0) {
		switch (optc) {
		  case 'a':
			address = strdup(optarg);
		  case 'i': // set file path
			inst_id = atoi(optarg);
			break;
		  case 'D': // Day
			day = atoi(optarg);
			break;
		  case 'l': // language
			language = endata_lang_get(optarg);
			break;
		  case 'p':
			select = atoi(optarg);
			break;
		  case 'd': // debug level
			deblev = atoi(optarg);
			break;
		  case 'h': // Help
		  default:
		  fprintf(stderr, "%s", HelpText);
			exit(0);
			break;
		}
	}
	
	struct endata_block *block = endata_get(inst_id, day, language, select, address, deblev);

	endata_block_print(block);

	endata_block_delete(block);


	free(address);

	exit(0);

}






