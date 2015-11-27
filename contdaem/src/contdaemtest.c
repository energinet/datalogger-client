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

#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <sys/stat.h> //pidfile
#include <sys/types.h> //kill
#include <signal.h>  //kill
#include <assert.h>

#include "module_util.h"



struct fval{
	float val;
	struct fval *next;
};


struct fval *fval_create(float value)
{
	struct fval *new = malloc(sizeof(*new));
	assert(new);

	new->val = value;
	new->next = NULL;

	return new;
}

struct fval *fval_lst_add(struct fval *list, struct fval *new)
{
	struct fval *ptr = list;

    if(!list){
        return new;
    }

    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;
    return list;
}


void fval_delete(struct fval *val){
	if(!val)
		return;

	fval_delete(val->next);

	free(val);
}

void fval_print(struct fval *list, FILE *stream){
	struct fval *ptr = list ;
	
	while(ptr){
		fprintf(stream, "%f;", ptr->val);
		ptr = ptr->next;
	}

	fprintf(stream, "\n");
}

struct fval *fval_parse_str(const char *str){
	struct fval *list = NULL;
	const char *ptr = str;
	float value;
	while(sscanf(ptr, "%f", &value)==1){
		list = fval_lst_add(list, fval_create(value));
		ptr = strchr(ptr, ',');
		if(ptr == NULL)
			break;
		ptr++;
	}

	return list;

	
}


void do_calc(struct module_calc *calc, struct fval *fvallst ,FILE *stream)
{
	struct fval *ptr = fvallst ;
	
	while(ptr){
		fprintf(stream, "%f => %f\n", ptr->val, module_calc_calc(calc,ptr->val));
		ptr = ptr->next;
	}


}

const  char *HelpText =
 "contdaemtest: Program for unit testing contdaem functions\n"
 " -v <value list>  : A list of values used for calc testing\n"
 " -c <calc>        : Test a calc function\n"
 " -d<level>        : Ses the debug level default: 1\n"
 " -h               : Help (this text)\n"
 "Christian Rostgaard, April 2013\n" 
 "";


int main(int narg, char *argp[])
{
    int dbglev = 1;
    int optc;

	struct fval *fvallst = NULL;
	struct module_calc *calc = NULL;	

    while ((optc = getopt(narg, argp, "c:v:d::h")) > 0) {
		switch(optc){
		  case 'v':
			fvallst = fval_parse_str(optarg);
			fval_print(fvallst, stdout);
			break;
		  case 'c':
			calc  = module_calc_create(optarg, dbglev);
			break;
		  case 'd':
			dbglev = atoi(optarg);
			break;
		  default:
			fprintf(stderr, "%s", HelpText);
			exit(0);
			break;
		}

	}

	do_calc(calc,fvallst ,stdout);

	return 0;
}


