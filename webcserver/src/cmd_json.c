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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <syslog.h>
#include <asocket_trx.h>
#include <asocket_client.h>
#include <qDecoder.h>
#include <errno.h>
#include <cmddb.h>

#include "json.h"
#include "siteutil.h"

const char* exclude_lst[] = {"confset", "jffsupd", "reboot", NULL};

int cmd_excluded(const char * name)
{
	int i = 0;
	
	while(exclude_lst[i]){
		if(strcmp(name, exclude_lst[i++])==0)
			return 1;
	}

	return 0;
	
}



struct json *cmd_set(struct json *json)
{

	struct json *ptr = (json)?json->childs:NULL;
	char value[512];
	struct json *jsonret = json_create_array(NULL);
	time_t t_now = time(NULL);
	time_t t_exe = time(NULL);

	while(ptr){
		memset(value,0, sizeof(value));

		cmddb_insert(0, ptr->name, ptr->value, t_now , 0, t_now);

		t_exe = cmddb_value(ptr->name, value , sizeof(value), t_now);
		struct json *jsonele = json_create_obj(NULL);
		json_add_child(jsonele, json_create_str("id", ptr->name));
		json_add_child(jsonele, json_create_time("exe_time", t_exe));
		json_add_child(jsonele, json_create_str("value", value));

		json_add_child(jsonret, jsonele);
		
		ptr = ptr->next;
	}
	
	return jsonret;
}


struct json *cmd_get(struct json *json)
{

	struct json *ptr = (json)?json->childs:NULL;
	char value[512];
	struct json *jsonret = json_create_array(NULL);
	time_t t_now = time(NULL);
	time_t t_exe = time(NULL);

	while(ptr){
		memset(value,0, sizeof(value));
		t_exe = cmddb_value(ptr->value, value , sizeof(value), t_now);
		struct json *jsonele = json_create_obj(NULL);
		json_add_child(jsonele, json_create_str("id", ptr->value));
		json_add_child(jsonele, json_create_time("exe_time", t_exe));
		json_add_child(jsonele, json_create_str("value", value));

		json_add_child(jsonret, jsonele);
		
		ptr = ptr->next;
	}
	
	return jsonret;
}


struct json *cmd_list(struct json *json)
{
	struct json *jsonret = json_create_array(NULL);
	struct cmddb_desc *list = cmddb_desc_get_list();
	struct cmddb_desc *ptr = list;

	while(ptr){
		if(!cmd_excluded(ptr->name)){
			struct json *jsonele = json_create_obj(NULL);
			json_add_child(jsonele, json_create_str("id", ptr->name));
			json_add_child(jsonele, json_create_str("text", ptr->name));
			json_add_child(jsonele, json_create_str("write", "true"));
			json_add_child(jsonret, jsonele);
		}
			
		ptr = ptr->next;
	}
	
	cmddb_desc_delete(list);

	return jsonret;
}


int main(int argc, char *argv[])
{
	struct sitereq site;
	int iret = 0;
	struct json *json = NULL;
	struct json *jsonret = NULL;
	cmddb_db_open(DEFAULT_CMD_LIST_PATH, 0);	

	siteutil_init_notpe(&site, "CMD-json", "cmd_json");

	openlog("cmd_json.cgi", LOG_PID, LOG_DAEMON|LOG_PERROR);

	setlogmask(LOG_UPTO(LOG_DEBUG));
	qCgiResponseSetContentType(site.req, "application/json");
	

	const char *list = site.req->getStr(site.req, "list", false);

	if(list){
		json = json_parse(NULL, list, &iret, strlen(list));
		jsonret = json_list_add(jsonret, cmd_list(json));
	}

	const char *set = site.req->getStr(site.req, "set", false);

	if(set){
		json = json_parse(NULL, set, &iret, strlen(set));
		jsonret = json_list_add(jsonret, cmd_set(json));
	}

	
	const char *get = site.req->getStr(site.req, "get", false);

	if(get){
		json = json_parse(NULL, get, &iret, strlen(get));
		jsonret = json_list_add(jsonret, cmd_get(json));
	}
		
	json_print(stdout, jsonret, FALSE);


	siteutil_deinit(&site);
	cmddb_db_close();
 			   
    syslog(LOG_DEBUG, "Exported");
    return EXIT_SUCCESS;

}
