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
#include "kamcom_types.h"

#include <math.h>
#include <stdio.h>
#include "stdlib.h"

struct kamcom_text{
    unsigned short num;
    const char *str;
};
/*
0: '', 1: 'Wh', 2: 'kWh', 3: 'MWh', 4: 'GWh', 5: 'j', 6: 'kj', 7: 'Mj',
	8: 'Gj', 9: 'Cal', 10: 'kCal', 11: 'Mcal', 12: 'Gcal', 13: 'varh',
	14: 'kvarh', 15: 'Mvarh', 16: 'Gvarh', 17: 'VAh', 18: 'kVAh',
	19: 'MVAh', 20: 'GVAh', 21: 'kW', 22: 'kW', 23: 'MW', 24: 'GW',
	25: 'kvar', 26: 'kvar', 27: 'Mvar', 28: 'Gvar', 29: 'VA', 30: 'kVA',
	31: 'MVA', 32: 'GVA', 33: 'V', 34: 'A', 35: 'kV',36: 'kA', 37: 'C',
	38: 'K', 39: 'l', 40: 'm3', 41: 'l/h', 42: 'm3/h', 43: 'm3xC',
	44: 'ton', 45: 'ton/h', 46: 'h', 47: 'hh:mm:ss', 48: 'yy:mm:dd',
	49: 'yyyy:mm:dd', 50: 'mm:dd', 51: '', 52: 'bar', 53: 'RTC',
	54: 'ASCII', 55: 'm3 x 10', 56: 'ton x 10', 57: 'GJ x 10',
	58: 'minutes', 59: 'Bitfield', 60: 's', 61: 'ms', 62: 'days',
	63: 'RTC-Q', 64: 'Datetime'
*/
struct kamcom_text units[] = {
    { 0x01 , "Wh"  },
    { 0x02 , "kWh" },
    { 0x03 , "MWh" },
    { 0x04 , "GWh" },
    { 0x05 , "j"   },
    { 0x06 , "kj" },
    { 0x07 , "Mj" },
    { 0x08 , "Gj"  },  
    { 0x0C , "Gcal"},
    { 0x17 , "KW"  },
    { 0x22 , "A"  },
    { 0x25 , "°C"  },
    { 0x26 , "K" },
    { 0x27 , "l" },
    { 0x28 , "m³" },
    { 0x29 , "l/h" },
    { 0x2A , "m³/h" },
    { 0x2B , "m³x°C" },
    { 0x2C , "ton" },
    { 0x2D , "ton/h" },
    { 0x2E , "h" },
    { 0x2F , "clock" },
    { 0x30 , "date1" },
    { 0x32 , "date3" },
    { 0x33 , "number" },
    { 0x34 , "bar" },
    { 0 , NULL }

};


const char* kamcom_types_getstr(struct kamcom_text *lst, unsigned short num){
    int ptr = 0;
    do{
	if(lst[ptr].num == num)
	    return lst[ptr].str;
    } while (lst[++ptr].str);

    return NULL;
}


const char* kamcom_types_unit(unsigned char unit)
{
    return kamcom_types_getstr(units, (unsigned char)unit);
}


#define KAMCOM_SIEX_SI (1 << 7)
#define KAMCOM_SIEX_SE (1 << 6)
#define KAMCOM_SIEX_EX (0x3f)


float kamcom_types_number(unsigned char siex, unsigned char size, unsigned char *data)
{
    unsigned int value = 0;
    int i ;
    int exp = (siex & KAMCOM_SIEX_EX);

    if(siex&KAMCOM_SIEX_SE){
	exp *= -1;
    }

    for(i = 0; i < size;  i++){
	value = value << 8;
	value += data[i];
    }

    if(siex&KAMCOM_SIEX_SI){
	value *= -1;
    }

    return ((float)value)*pow(10, exp);

}

int kamcom_types_int(unsigned char siex, unsigned char size, unsigned char *data ) {

	unsigned int value = 0;
	int i;

    for(i = 0; i < size;  i++){
		value = value << 8;
		value += data[i];
    }

    if(siex&KAMCOM_SIEX_SI){
		value *= -1;
    }

	return value;
}
