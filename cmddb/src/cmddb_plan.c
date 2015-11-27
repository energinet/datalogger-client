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
#include "cmddb_plan.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sqlite3.h>
#include <stdlib.h>

extern int dbglev;
extern struct sqlite3 *cdb;

int cmddb_mark_remove_list(const char *list, const char *colid);


/***********************************
 * Command database plan functions */

struct cmddb_plan *cmddb_plan_create(int rowid, time_t ttime, const char *value)
{
	struct cmddb_plan *new = malloc(sizeof(*new));
	assert(new);

	new->rowid = rowid;
	new->ttime = ttime;
	
	if(value)
		new->value = strdup(value);
	else
		new->value = NULL;
	new->next = NULL;
	
	return new;
}


struct cmddb_plan *cmddb_plan_add(struct cmddb_plan *list ,struct cmddb_plan *new)
{
	struct cmddb_plan *ptr = list;

    if(!list)
        /* first module in the list */
        return new;
	
    /* find the last module in the list */
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}

struct cmddb_plan *cmddb_plan_rem(struct cmddb_plan *list ,struct cmddb_plan *rem)
{
	struct cmddb_plan *ptr = list;
	struct cmddb_plan *prev = NULL;

    /* find the last module in the list */
    while(ptr){
        if(rem == ptr){
            if(prev)
                prev->next = ptr->next;
            else
                list = ptr->next;
            
            ptr->next = NULL;
            break;
        }
        prev = ptr;
        ptr = ptr->next;
    }

    return list;

}





void cmddb_plan_delete(struct cmddb_plan *plan)
{
	if(!plan)
		return;
	
	cmddb_plan_delete(plan->next);

	free(plan->value);

	free(plan);
}


void cmddb_plan_print(struct cmddb_plan *plan)
{
	int count = 0;
	printf("plan: %p\n", plan);

	while(plan){
		printf("%p %2d: %5.5d  %s : %s", plan , count++, plan->rowid, plan->value, ctime(&plan->ttime));
		
		plan = plan->next;
	}

}

struct cmddb_plan *cmddb_plan_find(struct cmddb_plan *plan, time_t ttime)
{
	while(plan){
		/* fprintf(stderr, "plan%p %ld==%ld\n", plan, plan->ttime , ttime); */
		if(plan->ttime == ttime)
			return plan;

		plan = plan->next;
	}

	return NULL;
}


int cmddb_plan_prune(struct cmddb_plan **new_plan__, struct cmddb_plan **old_plan__)
{

	struct cmddb_plan *new_plan = *new_plan__;	
	struct cmddb_plan *old_plan = *old_plan__;
	struct cmddb_plan *ptr = new_plan;
	int pcount = 0;

	if(!old_plan)
		return 0;

	if(!new_plan)
		return 0;

	while(ptr){
		struct cmddb_plan *res = cmddb_plan_find(old_plan, ptr->ttime);

		if(res){
			if(strcmp(res->value, ptr->value)==0){
//				fprintf(stderr, "del: ptr(%p):%s, res(%p):%s \n", 
//						ptr,  ptr->value, res, res->value );
				struct cmddb_plan *rem = ptr;
				ptr = ptr->next;				
				old_plan = cmddb_plan_rem(old_plan , res);
				cmddb_plan_delete(res);
				new_plan = cmddb_plan_rem(new_plan , rem);
				cmddb_plan_delete(rem);
				pcount++;
		
				continue;
			} else {
				/* fprintf(stderr, "dif: ptr(%p):%s, res(%p):%s \n",  */
				/* 		ptr,  ptr->value,res, res->value ); */
			}
		} else {
				/* fprintf(stderr, "non: ptr(%p):%s,  \n",  */
				/* 		ptr, ptr->value ); */
		}

		ptr = ptr->next;
	}

	*new_plan__ = new_plan;
	*old_plan__ = old_plan;

	return pcount;
}



int cmddb_plan_set__(struct cmddb_plan *plan, const char *cmd_name)
{
	while(plan){
		cmddb_insert(0, cmd_name, plan->value, plan->ttime, 0, time(NULL));
		plan = plan->next;
	}
	
	return 0;
}

int cmddb_plan_rem__(struct cmddb_plan *plan, const char *cmd_name)
{
	char list[512] = "";
	int len = 0;
	int retval = 0;

	assert(cmd_name);
	

	while(plan){

		if(len >= (sizeof(list)-10)){
			fprintf(stderr, "Error : Upper limit removal \n");
			retval = -1;
			break;
		}

		if(len)
			list[len++] = ',';
		
		len += snprintf(list+len, sizeof(list)-len, "%d", plan->rowid);
		
		plan = plan->next;
	}
		

	cmddb_mark_remove_list(list, "rowid");

	return retval;
}




/**
 * Write a plan to the database
 */
int cmddb_plan_set(struct cmddb_plan *plan, const char *cmd_name, time_t t_begin, time_t t_end)
{
	struct cmddb_plan *old_plan = NULL;
	int retval = 0;
	
	assert(cmd_name);

	if(t_begin&&t_end){
		old_plan = cmddb_plan_get(cmd_name, t_begin, t_end);
		cmddb_plan_prune(&plan, &old_plan);
	} 

	if(dbglev){
		cmddb_plan_print(plan);
		cmddb_plan_print(old_plan);
	}
	
	if((retval = cmddb_plan_set__(plan, cmd_name))<0){
		fprintf(stderr, "Error %d: Could not set new plan \n", retval);
		return retval;
	}		
	

	if((retval = cmddb_plan_rem__(old_plan, cmd_name))<0){
		fprintf(stderr, "Error %d: Could not set new plan \n", retval);
		return retval;
	}

	cmddb_plan_delete(plan);
	cmddb_plan_delete(old_plan);
	

	return 0;

}


struct cmddb_plan *cmddb_plan_get(const char *cmd_name, time_t t_begin, time_t t_end)
{
	sqlite3_stmt *stathandel;
    const char *pzTail;
    char query[512];
    int len, retval;
    struct cmddb_plan *list = NULL;

	len = snprintf(query, sizeof(query),
				   "SELECT rowid, ttime, value FROM "DB_TABLE_NAME_CMD_LST" WHERE "
				   "ttime >= %ld AND ttime <= %ld AND name = '%s' AND status = 1 ORDER BY ttime ASC", 
				   t_begin, t_end, cmd_name);

    
    retval = sqlite3_prepare_v2(cdb, query,  len , &stathandel, &pzTail );
    
    if(retval != SQLITE_OK){
		fprintf(stderr, "Error %d: Could not prepare statement \"%s\" ", retval, query);
		return NULL;
    }
	
    while(sqlite3_step(stathandel) == SQLITE_ROW){
		struct cmddb_plan *new;
		new = cmddb_plan_create(sqlite3_column_int(stathandel, 0), /* rowid */
 								sqlite3_column_int(stathandel, 1), /* ttime */ 
								(const char*)sqlite3_column_text(stathandel, 2) /*value*/);

		list = cmddb_plan_add(list, new);

    }
	
    sqlite3_finalize(stathandel);

    return list; 
    
}


