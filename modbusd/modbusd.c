/* Libary: Linux Demo Application
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Original file, demo.c modified by LIAB ApS 2009
 */

/* ----------------------- Standard includes --------------------------------*/
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

#include "mb.h"
#include "mbport.h"
#include "mbproto.h"
#include "mb-mastfunc.h"
#include "mbframe.h"

#include "msgpipe.h"

#include "pidfile.h"

#define DEFAULT_PIDFILE "/var/run/modbusd.pid"

#define uuDBG
#define MAXLEN 255
#define DEFAULT_TTY  "/dev/ttyS4"
#define DEFAULT_BAUD 38400
#define DEFAULT_PAR  "none"


#define PRINT_DBG(lev, str,arg...) {if(dbglev<=lev){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}



#define DEFAULT_ADDRESS 0x10
#define DEFAULT_REGISTER_START 1000

#define xstr(s) str(s)
#define str(s) #s


int dbglev = 0;

static char *helptext = 
  "\n"
  "MODBUS master program."
  "\n"
  "Usage: modbus [OPTIONS]\n"
  "\n"
  "Arguments:"
  "\n"
  "  -h, --help           Help text (this text)\n"
  "  -v, --version        Prints version info\n"
  "\n"
  "Serial setup\n"
  "  -d, --device=DEV     Specifies serial device (default "DEFAULT_TTY")\n"
  "  -s, --speed          Specify the baud rate (default "xstr(DEFAULT_BAUD)")\n"
  "  -p, --parity         Specify parity (default "DEFAULT_PAR")\n"
  "\nLIAB ApS 2009 <www.liab.dk>\n\n";

static struct option long_options[] =
  {
    {"help",       no_argument,             0, 'h'},
    {"version",    no_argument,             0, 'V'},
    {"device",     required_argument,       0, 'd'},
    {"speed",      required_argument,       0, 's'},
    {"parity",     required_argument,       0, 'p'},
    {0, 0, 0, 0}
  };
 

enum eRegAction{ACTION_INVALID=-1, 
                ACTION_READ, 
                ACTION_WRITE, 
                ACTION_WRITEMULTIPLE};

int parse_no_reg(const char *cmd);
int parse_address(const char *cmd);
int parse_reg_action(const char *cmd, int *val);
enum eRegTypes parse_reg_type(const char *cmd);
int parse_reg_start(const char *cmd);

/* ----------------------- Implementation -----------------------------------*/

static int running = 1;
static int outer_loop = 1;



void int_handler()
{
  running = 0;
  outer_loop = 0;
}

int main( int argc, char *argv[] )
{
  int c;
  int option_index = 0;
  int received;
  int mq_id;
  struct sIPCMsg mymsg;

  char *ttydev = DEFAULT_TTY;
  char *parity_s = DEFAULT_PAR;
  int parity;
  int baud = DEFAULT_BAUD;
  USHORT result[255] = {0,};
  int result_int = 0;

  USHORT usLen;
  eMBErrorCode err = MB_ENOERR;

  int reg_start = DEFAULT_REGISTER_START;
  int address = DEFAULT_ADDRESS;
  int regt = -1;
  int reg_action = -1;
  int reg_action_param = -1;
  int no_reg = -1;
  int run_forground = 0;
  int errorcount = 0;
  struct pidfile *pidfile = NULL;

  UCHAR *pucFrame;
  eMBException    eException;
  UCHAR ucFuncType;

  /* Parse cmd-line options */
  while ( (c = getopt_long (argc, argv, "hVd:s:p:r:t:c:a:f", long_options, &option_index)) != EOF ) {    
    switch (c) {
      case 'h':
        fprintf(stderr, "%s", helptext);
        exit(EXIT_SUCCESS);
      case 'V':
        exit(EXIT_SUCCESS);
      case 'd':
        ttydev = strndup(optarg, MAXLEN);
        break;
      case 's':
        baud = atoi(optarg);
        break; 
      case 'p':
        parity_s = strndup(optarg, MAXLEN);
        break;
      case 'f': // run in forground
	run_forground = 1;
	break;
      case '?':
        /* getopt_long already printed an error message. */
      default:
        fprintf(stderr, "%s", helptext);
        exit(EXIT_SUCCESS);
    }
  }

  if(!run_forground){
      fprintf(stderr, "Deamonizing...\n");
      daemon(0, 0);
  }


  /* Setup signalhandler so that we shut down 
     nicely on CTRL+C */
  signal(SIGINT, int_handler);
  
  pidfile = pidfile_create(DEFAULT_PIDFILE, PMODE_RETURN, 0);

  mq_id = create_ipc();


  /* Allocate space for an ADU frame */
  pucFrame = eMBAllocateFrame(&usLen);

  while(outer_loop) {

    if(running == 1) 
      printf("Restarting modbus layer\n");   
    else {
      printf("\n\n\n\n Restarting modbus layer\n\n\n\n");
      running = 1;
      errorcount = 0;
    }

    /* Parse user inputs... */
    if(strcmp(parity_s, "even") == 0)
      parity = MB_PAR_EVEN;
    else if(strcmp(parity_s, "odd") == 0)
      parity = MB_PAR_ODD;
    else
      parity = MB_PAR_NONE;

    PRINT_DBG(1, "init modbus at tty: %s parity %d baud %d", ttydev, parity, baud);

    /* mode, port, baud, parity */
    eMBMasterInit(MB_RTU, ttydev, baud, parity);

    while(running) {

      /* Check message queue */
      if((received = msgrcv(mq_id, &mymsg, sizeof(mymsg.text), 1, IPC_NOWAIT)) > 0) {

        /* Address */
        address = parse_address(mymsg.text);
        /* Starting register */
        reg_start = parse_reg_start(mymsg.text);
        /* Register type */
        regt = parse_reg_type(mymsg.text);
        /* Register action */
        reg_action_param = 0;
        reg_action = parse_reg_action(mymsg.text, &reg_action_param);
        /* Optional: number of registers */
        no_reg = parse_no_reg(mymsg.text);


#ifdef DBG
	if(dbglev>=1){
	    printf("Got: %s\n", mymsg.text);
	    printf("Parsing command:\n");
	    printf("Address: 0x%02x\n", address);
	    printf("Register: %d\n", reg_start);
	    printf("Register type: %d\n", regt);
	    printf("Register action: %d  Arg: %d\n", reg_action, reg_action_param);
	    printf("Register count: %d\n", no_reg);
	}
#endif

        switch(regt) {
          case MB_TYPE_COILS:
            switch(reg_action) {
              case ACTION_READ:
                err = build_eMBMasterReadCoils (pucFrame, reg_start, reg_action_param, &usLen);
                break;
              case ACTION_WRITE:
                err = build_eMBMasterWriteCoils (pucFrame, reg_start, reg_action_param ? 0xff : 0x0, &usLen);
                break;
              case ACTION_WRITEMULTIPLE:
                err = build_eMBMasterWriteMultipleCoils (pucFrame, reg_start, no_reg, reg_action_param ? 0xff : 0x0, &usLen);
                break;
            }
            break;
          case MB_TYPE_INPUT:
            err = build_eMBMasterReadInput (pucFrame, reg_start, reg_action_param, &usLen);
            break;
          case MB_TYPE_HOLDING:
            switch(reg_action) {
              case ACTION_READ:
                err = build_eMBMasterReadHolding (pucFrame, reg_start, reg_action_param, &usLen);
                break;
              case ACTION_WRITE:
                err = build_eMBMasterWriteSingleHolding (pucFrame, reg_start, reg_action_param, &usLen);
                break;
            }
            break;
        }
     
#ifdef DBG
        if(dbglev){
          int n;
          printf("Dumping built frame:\n");
          for(n=0; n<10; n++)
            printf("%02x ",pucFrame[n]);
          printf("\n");            
        }
#endif
      
        /* ? */
        eMBEnable();

        memset(result, 0, sizeof(result));

        if(err == MB_EX_NONE) {
          /* Set the address of the device in question */
          eMBSetSlaveAddress(address);

          /* Send the frame */
          if( (err = eMBSendFrame(pucFrame, usLen)) == MB_ENOERR) {
            printf(" ---- done processing frame ----\n");
            eException = MB_EX_ILLEGAL_FUNCTION;
            ucFuncType = pucFrame[MB_PDU_FUNC_OFF];      

#ifdef DBG
	    if(dbglev){
            printf("%s():%d - after send...\n", __FUNCTION__, __LINE__);
            {
              int n;
              printf("Dumping received frame:\n");
              for(n=0; n<10; n++)
                printf("%02x ",pucFrame[n]);
              printf("\n");            
            }
	    }
#endif
          

            if(ucFuncType & 0x80) {
              UCHAR ucExceptType = pucFrame[MB_PDU_FUNC_OFF+1];
              /* We encountered some sort of error */
              switch(ucExceptType) {
                case MB_EX_ILLEGAL_FUNCTION:
                  fprintf(stderr, "Error! Illegal function!\n");
                  break;
                case MB_EX_ILLEGAL_DATA_ADDRESS:
                  fprintf(stderr, "Error! Illegal data address!\n");
                  break;
                case MB_EX_ILLEGAL_DATA_VALUE:
                  fprintf(stderr, "Error! Illegal data value!\n");
                  break;
                case MB_EX_SLAVE_DEVICE_FAILURE:
                  fprintf(stderr, "Error! Slave device failure!\n");
                  break;
                default:
                  fprintf(stderr, "Unknown error (%d)!\n", eException);
                  break;
              }
            } else {
              switch(ucFuncType) {
                case MB_FUNC_READ_COILS:
                  eException = parse_eMBMasterReadCoils(pucFrame, &usLen, (void *)result);
#ifdef DBG
                  if(dbglev)
		      printf("Reply: %04x\n", result[0]);
#endif
                  break;
                case MB_FUNC_WRITE_SINGLE_COIL:
                  eException = parse_eMBMasterWriteCoils(pucFrame, &usLen, (void *)result);
#ifdef DBG
		  if(dbglev)
		      printf("Reply: %04x\n", result[0]);
#endif
                  break;
                case MB_FUNC_READ_INPUT_REGISTER:
                  eException = parse_eMBMasterReadInput(pucFrame, &usLen, (void *)result);
#ifdef DBG
		  if(dbglev)
		      printf("Reply (%d): %04x\n", eException, result[0]);
#endif
                  break;
                case MB_FUNC_READ_HOLDING_REGISTER:
                  eException = parse_eMBMasterReadHolding(pucFrame, &usLen, (void *)result);
		  
		 
		  
#ifdef DBG
                  printf("Reply: %04x %04x %d %d\n", result[0],result[1], result_int, reg_action_param);
#endif
                  break;
                case MB_FUNC_WRITE_REGISTER:
                  eException = parse_eMBMasterWriteSingleHolding(pucFrame, &usLen, (void *)result);
#ifdef DBG
                  printf("Reply: %04x\n", result[0]);
#endif
                  break;
                case MB_FUNC_WRITE_MULTIPLE_COILS:
                  usLen = 5;
                  eException =  parse_eMBMasterWriteMultipleCoils(pucFrame, &usLen, (void *)result);
                  if(eException == MB_EX_NONE)
                    printf("Write multiple coils SUCCESS!\n");
                  else
                    printf("Write multiple coils FAIL!\n");
                  break;
                default:
                  printf("Unknown function type : %x\n", ucFuncType);
                  printf("%d : %s()\n", __LINE__, __FUNCTION__);
                  {
                    int n;
                    for(n=0; n<10; n++)
                      printf("%02x ",pucFrame[n]);
                    printf("\n");            
                  }
                  break;
              }
	      switch(ucFuncType) {
		case MB_FUNC_READ_HOLDING_REGISTER:
		case MB_FUNC_READ_INPUT_REGISTER:
		   if(reg_action_param == 2){
		      printf("!!!!\n");
		      result_int = (result[0]<<16) | result[1];
		  }else {
		      result_int = result[0];
		  }
		  break;
		default:
		  result_int = result[0];
		  break;
	      }
              if(eException == MB_EX_NONE) {
		  snprintf(mymsg.text, sizeof(mymsg.text), "%ld:REG:%d:RESULT:%d",  time(NULL), reg_start, result_int);
              } else {
                snprintf(mymsg.text, sizeof(mymsg.text), "ERROR:Parsing frame");
              }
#ifdef DBG
              printf("%s():%d - reply: %s\n",
                     __FUNCTION__, __LINE__, mymsg.text);
#endif
              goto out;

            }
          } else {
            printf("Error sending frame. (%d)\n", err);
            snprintf(mymsg.text, sizeof(mymsg.text), "ERROR:Sending frame");
            if(errorcount > 10)
              running = 0;
            else
              errorcount++;
          }
        } else {
          printf("Error in frame arguments.\n");
          snprintf(mymsg.text, sizeof(mymsg.text), "ERROR:Wrong argument");
        }
      out:
        /* Set the reply */
        mymsg.type = 2;
#ifdef DBG
        printf("%s():%d - reply: %s\n",
               __FUNCTION__, __LINE__, mymsg.text);
#endif
        msgsnd(mq_id, &mymsg, sizeof(mymsg.text), IPC_NOWAIT);

        eMBDisable();
      } else {
        //printf("No messages. Sleeping (%d - %s)\n", received, received < 0 ? strerror(errno) :  "");
      } 
      usleep(1000);
    }
    //    free(pucFrame);
    eMBClose();
  }

  destroy_ipc(PROJ_ID);

  pidfile_delete(pidfile);

  return 0;
}

int parse_address(const char *cmd)
{
  char *start;
  int reg = -1;

  if((start = strstr(cmd, "ADDR:")) != NULL) {
    if(sscanf(start, "ADDR:%i|", &reg) != 1) {
      return (-1);
    }
    if(reg < 0 ||  reg > 0xffff) {
      return (-1);
    }
  }
  return reg;
}

int parse_reg_start(const char *cmd)
{
  char *start;
  int reg = -1;

  if((start = strstr(cmd, "REG:")) != NULL) {
    if(sscanf(start, "REG:%i|", &reg) != 1) {
      return (-1);
    }
    if(reg < 0 ||  reg > 0xffff) {
      return (-1);
    }
  }
  return reg-1;
}

int parse_no_reg(const char *cmd)
{
  char *start;
  int reg = -1;

  if((start = strstr(cmd, "NOREG:")) != NULL) {
    if(sscanf(start, "NOREG:%i", &reg) != 1) {
      return (-1);
    }
    if(reg < 0 ||  reg > 0xffff) {
      return (-1);
    }
  }
  return reg;
}

enum eRegTypes parse_reg_type(const char *cmd)
{
  char *start;
  int regt = MB_TYPE_UNKNOWN;

  if((start = strstr(cmd, "TYPE:")) != NULL) {
    start += strlen("TYPE:");

    if(strncmp(start, "coils", 5) == 0) {
       regt = MB_TYPE_COILS;
    } else if(strncmp(start, "input", 5) == 0) {
      regt = MB_TYPE_INPUT;
    } else if(strncmp(start, "holding", 7) == 0) {
      regt = MB_TYPE_HOLDING;
    } else if(strncmp(start, "slaveid", 7) == 0) {
      regt = MB_TYPE_SLAVEID;
    }
  }
  return regt;
}

int parse_reg_action(const char *cmd, int *val)
{
  int _val;
  char *start;
  int action = ACTION_INVALID;

  if((start = strstr(cmd, "ACTION:")) != NULL) {
    start += strlen("ACTION:");
    
    if(strncmp(start, "read", 4) == 0) {
      if(sscanf(start, "read:%i", &_val) == 1) {
        action = ACTION_READ;
        *val = _val;
      }
    } else if(strncmp(start, "writemultiple", 13) == 0) {
      if(sscanf(start, "writemultiple:%i", &_val) == 1) {
        action = ACTION_WRITEMULTIPLE;
        *val = _val;
      }
    } else if(strncmp(start, "write", 5) == 0) {
      if(sscanf(start, "write:%i", &_val) == 1) {
        action = ACTION_WRITE;
        *val = _val;
      }
    }
  }
  
  return action;
}
