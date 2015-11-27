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

#include "json.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct json *json_create_(void)
{
	struct json *new = malloc(sizeof(*new));
	assert(new);
	memset(new, 0, sizeof(*new));
	
	return new;
}



struct json *json_create(const char *name, const char *value, enum jsontype type)
{
	struct json *new = json_create_();

	if(name)
		new->name = strdup(name);

	if(value)
		new->value = strdup(value);

	new->type = type;

	return new;
}

struct json *json_create_str(const char *name, const char *value)
{
	return json_create(name, value, JSTEXT);
		
}

struct json *json_create_time(const char *name, time_t value)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%d000", (int)value);

	return json_create(name, buf, JSNUMBER);
	

	
}


struct json *json_create_int(const char *name, int value)
{
	char buf[32];

	snprintf(buf,sizeof(buf),  "%d", value);

	return json_create(name, buf, JSNUMBER);


	
}

struct json *json_create_obj(const char *name)
{
	return json_create(name, NULL, JSOBJECT);
	

}


struct json *json_create_array(const char *name)
{
	return json_create(name, NULL, JSARRAY);
	

}

struct json *json_list_add(struct json *list, struct json *new)
{
	new->next = list;
	return new;
}


struct json *json_list_add_(struct json *list, struct json *new)
{
	struct json *ptr = list;

    if(!list){
        /* first module in the list */
        return new;
    }

    /* find the last module in the list */
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;

    return list;
}


struct json *json_add_child(struct json *parent, struct json *child)
{
	assert(parent);
	parent->childs  = json_list_add(parent->childs, child);

	return parent;
}

void json_delete(struct json *rem)
{
	if(!rem)
		return;

	json_delete(rem->childs);
	json_delete(rem->next);
	
	free(rem->name);
	free(rem->value);
}

int json_trim(const char *str, int maxlen)
{
	int i = 0;
	
	while(i < maxlen){
		switch(str[i]){
		  case '\n':
		  case '\r':
		  case ' ':
			i++;
			continue;
		  default:
			return i;
		}
	}
	
	return i;
}


int json_getnum(const char *str, char *dst)
{
	int i = 0, n = 0;
	int len = strlen(str);
	
	while(i < len){
		switch(str[i]){
		  case ':': 
		  case '}':
		  case ']':
		  case ',':
			dst[n++] = '\0';
			return i;				
		  default:
			dst[n++] = str[i++];
			break;

		}
	}
	return i;
}

int json_getstr(const char *str, char *dst)
{
	int i = 0, n = 0;
	int len = strlen(str);

	while(i < len){
		switch(str[i]){
		  case '\\': /* esc */
			i++;
		  default:
				dst[n++] = str[i++];
				break;
		  case '"': /* end */
			dst[n++] = '\0';
			i++;
			return i;
		}
	}
	
	return i;
}

const char* json_setstr(const char *str, char *dst)
{
	int i = 0, n = 0;
	int len = strlen(str);

	while(i < len){
		switch(str[i]){
		  case '\\':
		  case '"': 
			i++;
			dst[n++] = '\\';
		  default:
			dst[n++] = str[i++];
			break;
		}
	}

	dst[n++] = '\0';

	return dst;
}



struct json *json_parse(struct json *lst, const char *str, int *iret, int maxlen)
{
	int i = 0, n= 0;
	char text[512] = "";
	enum jsontype type = JSOBJECT;

	if(!lst)
		lst = json_create_();

	while(i < maxlen){
		type = JSOBJECT;
		i += json_trim(str+i, maxlen-i);
		switch(str[i++]){
		  case '}':
		  case ']':
			goto out;
		  case '[':
			type = JSARRAY;
			/* pass through */
		  case '{': /* object */
			lst->type = type;
			lst->childs = json_parse(lst->childs, str+i, &i,  maxlen-i);
			if(lst->childs == NULL)
				return NULL;
			break;
			if(str[i]!='}' || str[i]!=']'  )
				return NULL;
			i++;
			break;
		  case ',': /* next */
			lst->next = json_parse(lst->next, str+i, &i,  maxlen-i);
			break;
		  case ':':
			lst->name = lst->value;
			lst->value = NULL;
			lst = json_parse(lst, str+i, &i,  maxlen-i);
			break;
		  case '"': /* text */
			type = JSTEXT;
			n = json_getstr(str+i, text);				
		  default:
			if(type != JSTEXT){
				i--;
				type = JSNUMBER;
				n = json_getnum(str+i, text);				
			}		
			if(n==0)
				return NULL;
			
			lst->type = type;
			i += n;
			lst->value = strdup(text);
			break;
		}
	}
  out:
	*iret += i;
	return lst;
}


void json_print(FILE *stream, struct json *json, int inArray)
{
	char text[512]; 
	char value[512]; 
	char chsta = '{';
	char chend = '}';
	if(!json)
		return;

	switch(json->type){
	  case JSARRAY:
		chsta = '[';
		chend = ']';
	  case JSOBJECT:
		if(json->name)
			fprintf(stream, "\"%s\":%c", json->name, chsta);		
		else 
			fprintf(stream, "%c",chsta );
		json_print(stream, json->childs, FALSE);
		fprintf(stream, "%c", chend);
		break;
	  case JSTEXT:
	  case JSDATA:
		if(json->name)
			fprintf(stream, "\"%s\":\"%s\"", json_setstr(json->name, text), json_setstr(json->value, value));		
		else
			fprintf(stream, "\"%s\"",json_setstr(json->value, value));		
		break;
	  case JSNUMBER:
		if(json->name)
			fprintf(stream, "\"%s\":%s", json_setstr(json->name, text), json_setstr(json->value, value));		
		else
			fprintf(stream, "%s",  json_setstr(json->value, value));		
		break;
	  default:
		break;
	}
	
	if(json->next){
		fprintf(stream, ",");
		json_print(stream, json->next, TRUE);
	} 
	
}



struct json *json_get_obj(struct json *lst, const char *name)
{
	struct json *ptr = lst;

	while(ptr){
		if(ptr->name)
			if(strcmp(ptr->name, name) == 0)
				return ptr;
		ptr = ptr->next;
	}

	return NULL;
}



struct json *json_get_childs(struct json *lst, const char *name)
{
	struct json *ptr = json_get_obj(lst, name);
	
	if(!ptr)
		return NULL;

	return  ptr->childs;

}

const char *json_get_value(struct json *lst, const char *name)
{
	struct json *ptr = json_get_obj(lst, name);
	
	if(!ptr)
		return NULL;

	return ptr->value;
}
