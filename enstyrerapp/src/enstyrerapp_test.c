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

#include "enstyrerapp.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>
#include <signal.h>
#include <endata.h>
#include <contlive.h>


int enapptest_endata(int inst_id, int dbglev)
{
	


	struct endata_block *block =  endata_get(inst_id, 0 , ENDATA_LANG_UK, ENDATA_ALL,NULL, dbglev);
	endata_block_print(block);

	endata_block_delete(block);

	return 0;
	
}


int enapptest_contlive(const char *varname, int dbglev)
{
	char value[64];
	char hname[64];
	char unit[64];

	struct asocket_con* sk_con = contlive_connect(NULL, 0, dbglev);
	contlive_get_single( sk_con, varname, value, hname, unit);
	
	printf("contlive: %s, %s, %s\n",  value, hname, unit);
	
	return 0;
}


