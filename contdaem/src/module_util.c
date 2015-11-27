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

#define NAN (-4000000)

struct lt {
	float y;
	float x;
};

struct lt tc_k[] = { 
	{-260, -0.786}, {-240, -0.774}, {-220, -0.751}, {-200, -0.719}, 
	{-180, -0.677}, {-160, -0.627}, {-140, -0.569}, {-120, -0.504}, {-100, -0.432},
	{ -80, -0.355}, { -60, -0.272}, { -40, -0.184}, { -20, -0.093},
	{   0,  0.003}, {  20,  0.100}, {  25,  0.125}, {  40,  0.200}, {  60,  0.301}, {  80,  0.402}, 
	{ 100,  0.504}, { 120,  0.605}, { 140,  0.705}, { 160,  0.803}, { 180,  0.901}, { 200,  0.999},
	{ 220,  1.097}, { 240,  1.196}, { 260,  1.295}, { 280,  1.396}, 
	{ 300,  1.497}, { 320,  1.599}, { 340,  1.701}, { 360,  1.803}, { 380,  1.906}, 
	{ 400,  2.010}, { 420,  2.113}, { 440,  2.217}, { 460,  2.321}, { 480,  2.425}, 
	{ 500,  2.529}, { 520,  2.634}, { 540,  2.738}, { 560,  2.843}, { 580,  2.947}, 
	{ 600,  3.051}, { 620,  3.155}, { 640,  3.259}, { 660,  3.362}, { 680,  3.465},
	{ 700,  3.568}, { 720,  3.670}, { 740,  3.772}, { 760,  3.874}, { 780,  3.975}, 
	{ 800,  4.076}, { 820,  4.176}, { 840,  4.275}, { 860,  4.374}, { 880,  4.473}, 
	{ 900,  4.571}, { 920,  4.669}, { 940,  4.766}, { 960,  4.863}, { 980,  4.959}, 
	{1000,  5.055}, {1020,  5.150}, {1040,  5.245}, {1060,  5.339}, {1080,  5.432}, 
	{1100,  5.525}, {1120,  5.617}, {1140,  5.709}, {1160,  5.800}, {1180,  5.891}, 
	{1200,  5.980}, {1220,  6.069}, {1240,  6.158}, {1260,  6.245}, {1280,  6.332}, 
	{1300,  6.418}, {1320,  6.503}, {1340,  6.587}, {1360,  6.671}, {1380,  6.754} 
};


struct ltable {
	char *name;
	//float *y;
	float *x;
	float *a;
	float *b;
	size_t size;
	size_t bitsize;
	struct ltable *next;
};



struct lt* module_calc_lt_from_str(const char *str, size_t *size_, int verbose)
{
	size_t i = 0;
	struct lt *tbl = malloc(sizeof(*tbl)*1000);
	
	const char * ptr = str;

	assert(tbl);

	while((ptr = strchr(ptr, '[')) != NULL){
		if(sscanf(ptr, "[%f:%f]", &(tbl[i].x), &(tbl[i].y))!=2){
			printf("ERR ltable: %s is not in the for of %s \n", ptr, "[%f:%f]");
			free(tbl);
			*size_ = 0;
			return NULL;
		}
		
		if(verbose >= 2)
			fprintf(stderr, "[%f:%f]\n", tbl[i].x, tbl[i].y);
		i++;
		ptr++;
	}
	
	if(verbose >= 2)
		fprintf(stderr, "(%zd)\n", i);
	
	*size_ = i;
	
	return tbl;
			
}

struct ltable *module_calc_ltable_create(const char *name, struct lt* tbl, size_t size_) {
	int i, size = size_, centbit = 0;
	struct ltable *new = malloc(sizeof(*new));
	assert(new);

	if (!name) {
		PRINT_ERR("No name\n!");
		free(new);
		return NULL;
	}

	new->name = strdup(name);

	new->size = size_;

	new->x = malloc(sizeof(float) * size);
	assert(new->x);

	for (i = 0; i < size; i++) {
		new->x[i] = tbl[i].x;
	}

	new->a = malloc(sizeof(float) * size);
	assert(new->a);
	new->b = malloc(sizeof(float) * size);
	assert(new->b);

	for (i = 0; i < size; i++) {
		//a = (y2-y1)/(x2-x1)
		new->a[i] =  (tbl[i].y-tbl[i + 1].y)/(new->x[i]-new->x[i + 1] );

		//b = y1 - (a*x1)
		if(new->a[i] != 0)
			new->b[i] = tbl[i].y - (new->a[i] * new->x[i]);
		else
			new->b[i] = tbl[i].y;

	}

	while (size > 0) {
		centbit++;
		size = size >> 1;
	}

	if (centbit > 0)
		new->bitsize = 1 << (centbit);
	else if (centbit == 1)
		new->bitsize = 1;
	else
		new->bitsize = 0;

	new->next = NULL;

	return new;
}


void module_calc_ltable_print(struct ltable *ltbl)
{
	assert(ltbl);
	int i;
	for(i = 0; i < ltbl->size; i++){
		fprintf(stderr, "x:%f a:%f b:%f\n", ltbl->x[i],ltbl->a[i],ltbl->b[i]);
	}


}


void module_calc_print(struct module_calc *calc, int verbose)
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
	   case LTABLE:
		 ctype = "ltab";
		 break;
	 }


	 fprintf(stderr, "%s:a=%e,b=%e,c=%e;",ctype, calc->a, calc->b, calc->c);

	 if(verbose >= 2 && calc->type ==LTABLE ){
		 fprintf(stderr, "\n");
		 module_calc_ltable_print(calc->calcobj);
	 }

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
    } else if (strstr(calc_str, "ltable") == calc_str) {
		new->type = LTABLE;
		if (verbose) {
			printf("calc: LTABLE\n");
		}
		if (strstr(calc_str, "tc_k") == calc_str + 7) {
			new->calcobj = module_calc_ltable_create("tc_k", tc_k, (size_t) 71);
			return new;
		} else {
			size_t size;
			struct lt *tbl = module_calc_lt_from_str(calc_str + 7, &size, verbose);
			new->calcobj = module_calc_ltable_create("cust", tbl, size);
			free(tbl);
			return new;
		}
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

float module_calc_calc(struct module_calc *calc, float input);

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
    
	module_calc_print(new, verbose);
    fprintf(stderr, "ret %p -> %p (%f)\n", new, new->next, module_calc_calc(new, 2800));
	
    return new;
}


int module_calc_ltable_find(struct ltable* table, float input)
{
	int i = 0;

	int bit = table->bitsize;
	int max = table->size - 1;

	do {
		bit = bit >> 1;
		i |= bit;

		if (i >= max || table->x[i] > input) {
			i &= ~bit;
		} else if (table->x[i + 1] > input) {
			return i;
		}

	} while (bit);

	return -1;
}

float module_calc_ltable_calc(struct ltable* table, float input)
{
	int cnt = module_calc_ltable_find(table,input);

	if(cnt == -1) {
		return NAN; //Value out of range!
	}

	return table->a[cnt]*input+table->b[cnt];
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
	case LTABLE:
		ret = module_calc_ltable_calc((struct ltable*)calc->calcobj,input);
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

int module_calc_calc_int(struct module_calc *calc, int input)
{
    int ret;

    if(!calc)
        return input;

    switch(calc->type){
      case CALC_POLY2:
        ret = (input*input)*calc->a + ((int)calc->b)*input + ((int)calc->c);
        break;
      case CALC_POLY1:
        ret =  input * ((int)calc->a) + ((int)calc->b);
        break;
      case CALC_THRH:
	if(input >= calc->b && input <= calc->a){
	    ret = 0;
	} else {
	    ret = input;
	}
	break;
	case LTABLE: //Not supported for integers
    default:
        ret = input;
        break;
    }

    

	if(calc->verbose)
        printf("calc: input %d , output %d\n", input, ret);

    ret = module_calc_calc_int(calc->next, ret);

    return ret;

}

/**
 * Flow to level conversions 
 * @ingroup module_util_ftol
 */
struct convunit ftolunits[] = { \
    { "cJ"    , "W"        , 1             , CONV_DIFF , 4 },
    { "_J"    , "W"        , 1             , CONV_DIFF , 4 },
    { "_m³"   , "m³/h"     , 3600          , CONV_DIFF , 4 },
    { "_l"    , "l/min"    , 60            , CONV_DIFF , 4 },
    { "_pulse", "pulse/sec", 1             , CONV_DIFF , 4 },
    { "°C"    , "°C"       , 1             , CONV_NONE , 4 },
    { "l/min" , "l/min"    , 1             , CONV_NONE , 4 },
    { "W"     , "W"        , 1             , CONV_NONE , 4 },
    { "_m"    , "m/s"      , 1             , CONV_DIFF , 4 },
    { NULL , NULL, 0},
};


struct convunit ltofunits[] = { \
    { "_J"    , "_kWh"     , 1.0/3600000.0 , CONV_NONE , 4 },
    { "_m³"   , "_m³"      , 1             , CONV_NONE , 4 },
    { "_l"    , "_l"       , 1             , CONV_NONE , 4 },
    { "l/min" , "_l"       , 1.0/60.0      , CONV_INTG , 4 },
    { "l/min" , "_m³"      , 1.0/60000.0   , CONV_INTG , 4 },
    { "W"     , "_kWh"     , 1.0/3600.0    , CONV_INTG , 4 },
    { NULL , NULL, 0},
};



struct convunit *module_conv_get(const char *unit_in, const char *unit_out, struct convunit *convunits)
{
    int i = 0; 

    if(!unit_in&&!unit_out)
	return NULL;
    
    fprintf(stderr,"in: %s, out %s\n", unit_in, unit_out);

    for(i = 0 ; convunits[i].unit_in ; i++){
	struct convunit *convunit = convunits + i;


	fprintf(stderr, "%p<<in: %s , out %s, %f, %d, %d\n", convunit,
		convunit->unit_in, convunit->unit_out, 
		convunit->scale, convunit->conv, convunit->decs);
		
	if(unit_in)
	    if(strcmp(convunit->unit_in, unit_in)!=0)
		continue;

	if(unit_out)
	    if(strcmp(convunit->unit_out, unit_out)!=0)
		continue;

	fprintf(stderr, "%p>>>in: %s , out %s, %f, %d, %d\n", convunit,
		convunit->unit_in, convunit->unit_out, 
		convunit->scale, convunit->conv, convunit->decs);
	    
	return convunit;

    }

    return NULL;
}


struct convunit *module_conv_get_flow(const char *unit_in, const char *unit_out)
{
    return module_conv_get(unit_in, unit_out, ltofunits);
}


struct convunit *module_conv_get_level(const char *unit_in, const char *unit_out)
{
    return module_conv_get(unit_in, unit_out, ftolunits);
}

float module_conv_calc(struct convunit *convunit, unsigned long msec, float value)
{
    if(!convunit)
	return value;

    value *= convunit->scale;

    switch(convunit->conv){
    case CONV_DIFF:
	if(msec > 0)
	    return ((value)/(((float)msec)/1000.0));
	else 
	    return 0 ;
    case CONV_INTG:
	return value * (msec/1000.0);
    case CONV_NONE:
    default:
	return value;
    }


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


