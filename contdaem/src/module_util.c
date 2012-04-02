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

#include "module_util.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <aeeprom.h>

 void module_calc_print(struct module_calc *calc)
 { 
	 if(!calc)
		 return ;

	 const char *ctype = "???";
	 switch(calc->type){
	   case CALC_POLY1:
		 ctype = "poly1";
		 break;
	   case CALC_POLY2:
		 ctype = "poly2";
		 break;
	   case CALC_THRH:
		 ctype = "thrh";
		 break;
	 }


	 fprintf(stderr, "%s:a=%e,b=%e,c=%e;",ctype, calc->a, calc->b, calc->c);
 } 


/**
 * Flow to level conversions 
 * @ingroup module_util_calc
 * @private
 */

struct module_calc *module_calc_create_single(const char *calc_str, int verbose)
{
    struct module_calc *new = NULL;
    char tmpbuf[5120];
    char fallback[5120];
	float a=0, b=0, c=0;
    fallback[0]= '\0';
    if(verbose)
        printf("calc: %s\n", calc_str);

    if(!calc_str)
        return NULL;

    new = malloc(sizeof(*new));
    assert(new);
    memset(new, 0, sizeof(*new));
    new->verbose = verbose;

    if(sscanf(calc_str, "poly1:a%fb%f", &a,  &b)==2){
        new->type = CALC_POLY1;
		new->a = a;  
		new->b = b;
        if(verbose)
            printf("calc: ax+b -- a= %f, b= %f (%f, %f)\n", new->a, new->b, a, b);
        return new;
    } else if(sscanf(calc_str, "poly2:a%fb%fc%f", &a,  &b, &c)==3) {
        new->type = CALC_POLY2;
		new->a = a;  
		new->b = b;
		new->c = c;  
        if(verbose)
            printf("calc: ax^2+bx+c -- a= %f, b= %f, c= %f \n", new->a, new->b, new->c);
        return new;
    } else if(sscanf(calc_str, "thrhz:%f", &new->a )==1) {
	new->type = CALC_THRH;
	new->b = -new->a;
	if(verbose)
            printf("calc: threshold arround zero ±%f (%f)\n", new->a, new->b);
        return new;
    } else if(sscanf(calc_str, "eeprom:%[a-zA-Z0-9_-.]#%s", tmpbuf, fallback ) > 1){
		char *ee_calc_str = aeeprom_get_entry(tmpbuf);
		struct module_calc *calc = module_calc_create(ee_calc_str, verbose);	
		if(!ee_calc_str){
			fprintf(stderr, "ERR-> calc: %s not found in eeprom\n", tmpbuf);
			if(strlen(fallback)>0)
				calc = module_calc_create_single(fallback, verbose);
		}
		free(ee_calc_str);
		return calc;
    }
	
    printf("ERR calc: %s\n", calc_str);
    free(new);
    return NULL;

}


struct module_calc *module_calc_create(const char *calc_str, int verbose)
{
    char *ptr =  NULL;//strchr(calc_str, ';');
    struct module_calc *new = NULL;

    new = module_calc_create_single(calc_str, verbose);
    
    if(!new)
	return NULL;

    if((ptr =  strchr(calc_str, ';'))){
		new->next = module_calc_create(ptr+1,verbose);
    }
    
	module_calc_print(new);
    fprintf(stderr, "ret %p -> %p (%f)\n", new, new->next, module_calc_calc(new, 2800));
	
    return new;
}



float module_calc_calc(struct module_calc *calc, float input)
{
    float ret;

    if(!calc)
        return input;

    switch(calc->type){
      case CALC_POLY2:
        ret = (input*input)*calc->a + calc->b*input + calc->c;
        break;
      case CALC_POLY1:
        ret =  input*calc->a + calc->b;
        break;
      case CALC_THRH:
	if(input >= calc->b && input <= calc->a){
	    ret = 0;
	} else {
	    ret = input;
	}
	break;
      default:
        ret = input;
        break;
    }

    

	if(calc->verbose)
        printf("calc: input %f , output %f\n", input, ret);

    ret = module_calc_calc(calc->next, ret);

    return ret;

}

/**
 * Flow to level conversions 
 * @ingroup module_util_ftol
 */
struct ftolunit ftlunits[] = { \
    { "cJ", "W" , 1 , 1, 1},
    { "_J", "W" , 1 , 1, 1},
    { "_m³", "m³/h" , 3600 , 1 , 2 },
    { "_pulse", "pulse/sec", 1 , 1 },
    { "°C", "°C", 1 , 0, 1},
    { "l/min", "l/min", 1 , 0, 1},
    { "W", "W", 1 , 0, 1},
    { "_m", "m/s" , 1 , 1, 1},
    { NULL , NULL, 0},
};



struct ftolunit *module_calc_get_flunit(const char *funit, const char *lunit)
{
    int i = 0; 

    if(!funit&&!lunit)
	return NULL;

    for(i = 0 ; ftlunits[i].funit ; i++){
	
	if(lunit)
	    if(strcmp(ftlunits[i].lunit, lunit)!=0)
		continue;

	if(funit)
	    if(strcmp(ftlunits[i].funit, funit)!=0)
		continue;
	    
	return ftlunits + i;

    }

    return NULL;
}


float module_calc_get_level(unsigned long usec, float units, struct ftolunit *ftlunit)
{
    float scale = 1; 

    if(!ftlunit)
	return units;

    scale =  ftlunit->scale;

    if(ftlunit->diff){
	if(usec&&units)
	    return ((units)/(((float)usec)/1000))*scale;
	else 
	    return 0 ;
    } else {
	return scale * units;
    }
    return 0;
}


unsigned long modutil_timeval_diff_ms(struct timeval *now, struct timeval *prev)
{
    int sec = now->tv_sec - prev->tv_sec;
    int usec = now->tv_usec - prev->tv_usec;

    unsigned long diff;

    if( sec < 0 ) {
        PRINT_ERR("does not support now < prev");
        sec = 0;
    }

    diff = sec*1000;
    diff += usec/1000;

    return diff;
    
        
}


struct timeval * modutil_timeval_add_ms(struct timeval *time, int diff)
{
    
    time->tv_sec += diff/1000;
    int usec = ((diff%1000)*1000)+time->tv_usec;

    if(usec > 1000000){
	usec -= 1000000;
	time->tv_sec++;
    } else if(usec < 0){
	time->tv_sec--;
	usec -= 1000000;
    }
	
    time->tv_usec = usec;
    
    return time;
}


void modutil_sleep_nextsec(struct timeval *time)
{
    unsigned long next = (1000000-time->tv_usec)+1000;

    if(next > 1000000)
	next -= 100000;

    usleep(next);

}



int modutil_get_listitem(const char *text, int def, const char** items)
{
    int i = 0;
    if(!text)
	return def;
    
    while(items[i]){
	if(strcmp(text, items[i])==0)
	    return i;
	i++;
    }

    return def;
}


char *modutil_strdup(const char *str)
{
    if(!str)
	return NULL;

    return strdup(str);
}




int moduleutil_replace(const char *inbuf, char* outbuf, int maxoutsize, struct mureplace_obj *repl_lst, void *userdata)
{
    int len = 0;
    const char *in_ptr = inbuf;
    const char *in_ptr_end = in_ptr + strlen(in_ptr);
    const char *in_ptr_next;
    int repl_index = -1;
    int i = 0;

    if(!in_ptr)
        return -1;

    while(in_ptr < in_ptr_end){
        int cpy_size = 0;
        /* find first replace var */
        in_ptr_next = in_ptr_end;
        i = 0;
        repl_index = -1;
        while(repl_lst[i].search != NULL){
            char *tmp_ptr = strstr(in_ptr, repl_lst[i].search);
            if(tmp_ptr)
                if(tmp_ptr < in_ptr_next){
                    in_ptr_next = tmp_ptr;
                    repl_index = i;
                }
            i++;
        }
        
        cpy_size = in_ptr_next - in_ptr;

        memcpy(outbuf+len, in_ptr,  in_ptr_next-in_ptr);

        in_ptr +=  cpy_size;
        len += cpy_size;
        if( repl_index > -1){
            if(repl_lst[repl_index].callback)
                len += repl_lst[repl_index].callback(userdata, outbuf+len,  maxoutsize-len);
            in_ptr += strlen(repl_lst[repl_index].search);
        }

    }
    
    outbuf[len] = '\0';

    return len;
}


