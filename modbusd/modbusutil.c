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

#define  _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#include "msgpipe.h"


const  char *HelpText =
 "modbusutil: util for modbus daemon... \n"
 " -a <addr>       : Address\n"
 " -t <type>       : Register type (i: input, c:couls , h:holdings, s:slaveid)\n"
 " -r <reg>        : Register\n"
 " -s <value>      : Set value \n"
 " -g              : Get value \n"
 " -m <count>      : multiwrite \n"
 " -S <addr_end>   : Address scan. -a <addr> is the first scanned, \n"
 "                   <addr_end> is the last\n"
 " -R <reg_end>    : Register scan. -r <reg> is the first scanned, \n"
 "                   <reg_end> is the last\n"
 " -T <param>      : Test sequence: 'm' miltiwrite and 's' is sequence  \n"
 " -d <level>      : Debug level \n"
 " -h              : Help (this text)\n"
 "Christian Rostgaard, June 2010\n" 
 "";




enum eRegTypes reg_type_get(const char *str)
{
    if(!str)
	return MB_TYPE_UNKNOWN;
    
    switch(str[0]){
      case 'i':
	return MB_TYPE_INPUT;
      case 'c':
	return MB_TYPE_COILS;
      case 'h':
	return MB_TYPE_HOLDING;
      case 's':
	return MB_TYPE_SLAVEID;
      default:
	return MB_TYPE_UNKNOWN;
    }
}


int reg_scan(int mq_id, int addr, int  reg_type, int reg_start, int reg_stop)
{
    int i; 
     int value;
     int retval; 

     for(i=reg_start;  i <= reg_stop; i++){
     	 retval = read_cmd(mq_id, addr, reg_type , i, MV_TYPE_SHORT, &value, 0);
	 switch(retval){
	   case 1:
	     printf("Read value %d at %d\n", value, i);
	     break;
	   default:
	     printf("Retval  %d at %d\n", retval, i);
	     break;	     
	 }
	 
     }

     return 0;
    
}

int addr_scan(int mq_id, int a_start , int a_stop, int  reg_type,int reg_no) 
{ 
     int i; 
     int value;
     int retval; 

     for(i=a_start;  i <= a_stop; i++){
     	 retval = read_cmd(mq_id, i, reg_type, reg_no, MV_TYPE_SHORT, &value, 0);
	 switch(retval){
	   case 0:
	     printf("Contact at 0x%x\n", i);
	     break;
	   case 1:
	     printf("Read value %d at 0x%x\n", value, i);
	     break;
	   default:
	     printf("Timeout at 0x%x\n", i);
	     break;	     
	 }
	 
     }

     return 0;
} 

int test_sequence(int mq_id, int addr, int reg_no, int reg_type,  int mult, const char *param)
{
    int i, n;

    switch(param[0]){
      case 'm': 
	while(1){
	    i++;
	    write_cmd(mq_id, addr, reg_type, reg_no, mult, i%2, 1);
	}	
	break;
      case 's':
	while(1){
	    n = 0;
	    i++;
	    for(n = 0; n < 8;n++)
		write_cmd(mq_id, addr,reg_type, reg_no+n, 1, i%2, 1);
	}	
	break;
    }
	
}

int atoi_adv(const char *ascii)
{
    int value = 0;
    
    if(sscanf(ascii, "0x%x", &value)==1)
	return value;
    else if(sscanf(ascii, "%d", &value)==1)
	return value;

    return 0;
}

int main(int argc, char *argv[])
{
  int mq_id;

  int i, max;

  int optc;
  int debug_level = 0;
  int addr = 0x10; 
  int reg_no=1000; 
  int reg_type = MB_TYPE_COILS;
  int value = 0;
  int timeout = 500;
  int mult = 0;



  mq_id = connect_ipc();
  flush_msg_ipc(mq_id);

  while ((optc = getopt(argc, argv, "a:t:r:m:s:g::S:T:R:hd::")) > 0) {
        switch (optc){ 
	  case 'a': //address
	    addr = atoi_adv(optarg);
	    break;
	  case 't': //register type
	    reg_type = reg_type_get(optarg);
	    break;
	  case 'r': //register number
	    reg_no = atoi_adv(optarg);
	    break;
	  case 'm':
	    mult = atoi_adv(optarg);
	    break;
	  case 'g':{
	      enum eValueTypes val_type = MV_TYPE_SHORT;
	      if(optarg)
		  if(optarg[0]=='l')
		      val_type = MV_TYPE_LONG;
	      if(read_cmd(mq_id, addr, reg_type, reg_no, val_type, &value, 1)==1)
		  printf("Read value %d\n", value);
	      else{
		  fprintf(stderr, "Error reading  value \n");
		  return 1;
	      }
	  }break;
	  case 's': //set register
	    value = atoi_adv(optarg);
	    write_cmd(mq_id,addr, MB_TYPE_COILS, reg_no, mult, value, debug_level);
	    break;	    
	  case 'S':
	    
	    addr_scan( mq_id, addr, atoi_adv(optarg) , reg_type, reg_no); 
	    
	    break;
	  case 'R':
	    reg_scan(mq_id, addr, reg_type , reg_no, atoi_adv(optarg));
	    break;
	  case 'T':
	    test_sequence(mq_id,addr, reg_no, reg_type,  mult, optarg);
	    break;
	  case 'd':
	    debug_level  = 1;
	    break;
          case 'h': // Help
          default:
            fprintf(stderr, "%s", HelpText);
            exit(0);
            break;
        }
    }
    

 
  return EXIT_SUCCESS;
}
