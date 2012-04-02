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
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <getopt.h>

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbproto.h"
#include "mb-mastfunc.h"
#include "mbframe.h"
#include "msgpipe.h"

/* ----------------------- Defines ------------------------------------------*/
#define MAXLEN 255
#define DEFAULT_ADDRESS 0x10
#define DEFAULT_TTY  "/dev/ttyS4"
#define DEFAULT_BAUD 38400
#define DEFAULT_PAR  "even"

#define DEFAULT_REGISTER_START 1000

#define xstr(s) str(s)
#define str(s) #s

/* ----------------------- Static decls -------------------------------------*/
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
  "\n"
  "MODBUS\n"
  "  -a, --address        Device address (default "xstr(DEFAULT_ADDRESS)"))\n"
  "  -r, --register       Register start address (default "xstr(DEFAULT_REGISTER_START)")\n"
  "  -t, --type           Register type:\n"
  "                         coils\n"
  "                         input\n"
  "                         holding\n"
  "                         slaveid\n"
  "  -c, --command        Command to execute:\n"
  "                         *Coils:\n"
  "                            \"read NO_COILS\"\n"
  "                            \"writesingle ON|OFF\"\n"
  "                            \"writemultiple NO_COILS ON|OFF\"\n"
  "                         *Input:\n"
  "                            \"read NO_REGS\"\n"
  "                         *Holding:\n"
  "                            \"read NO_REGS\"\n"
  "                            \"writesingle VALUE\"\n"
  "\nLIAB ApS 2009 <www.liab.dk>\n\n";

static struct option long_options[] =
  {
    {"help",       no_argument,             0, 'h'},
    {"version",    no_argument,             0, 'V'},
    {"device",     required_argument,       0, 'd'},
    {"speed",      required_argument,       0, 's'},
    {"parity",     required_argument,       0, 'p'},
    {"register",   required_argument,       0, 'r'},
    {"type",       required_argument,       0, 't'},
    {"command",    required_argument,       0, 'c'},
    {"address",    required_argument,       0, 'a'},
    {0, 0, 0, 0}
  };
 
enum eRegTypes {MB_TYPE_COILS, MB_TYPE_INPUT, MB_TYPE_HOLDING, MB_TYPE_SLAVEID};

/* ----------------------- Implementation -----------------------------------*/

int main( int argc, char *argv[] )
{
  int c;
  int option_index = 0;

  char *ttydev = DEFAULT_TTY;
  char *parity_s = DEFAULT_PAR;
  int parity;
  int baud = DEFAULT_BAUD;
  int reg_start = DEFAULT_REGISTER_START;
  int address = DEFAULT_ADDRESS;
  char *reg_type = NULL;
  int regt = -1;
  char *reg_action = NULL;

  USHORT usLen;
  UCHAR *pucFrame;
  eMBErrorCode err = MB_ENOERR;

  /* Parse cmd-line options */
  while ( (c = getopt_long (argc, argv, "hVd:s:p:r:t:c:a:", long_options, &option_index)) != EOF ) {    
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
      case 'r':
        reg_start = (int)strtod(optarg, NULL);
        break;
      case 't':
        reg_type = strndup(optarg, MAXLEN);
        break;
      case 'c':
        reg_action = strndup(optarg, MAXLEN);
        break; 
      case 'a':
        address = (int)strtod(optarg, NULL);
        break;
      case '?':
        /* getopt_long already printed an error message. */
      default:
        fprintf(stderr, "%s", helptext);
        exit(EXIT_SUCCESS);
    }
  }

  if(argc <= 1) {
    fprintf(stderr, "%s", helptext);
    exit(EXIT_FAILURE);
  }


  /* Allocate space for an ADU frame */
  pucFrame = eMBAllocateFrame(&usLen);

  /* Parse user inputs... */
  if(strcmp(parity_s, "even") == 0)
    parity = MB_PAR_EVEN;
  else if(strcmp(parity_s, "odd") == 0)
    parity = MB_PAR_ODD;
  else
    parity = MB_PAR_NONE;

  if(reg_start < 0 ||  reg_start > 0xffff) {
    fprintf(stderr, "Error! Modbus register out of range. Must be within (0x0-0xffff)\n");
    exit(EXIT_FAILURE);
  }

  if(reg_type) {
    if(strcmp(reg_type, "coils" ) == 0) {
      regt = MB_TYPE_COILS;
    } else if(strcmp(reg_type, "input" ) == 0) {
      regt = MB_TYPE_INPUT;
    } else if(strcmp(reg_type, "holding" ) == 0) {
      regt = MB_TYPE_HOLDING;
    } else if(strcmp(reg_type, "slaveid" ) == 0) {
      regt = MB_TYPE_SLAVEID;
      build_eMBMasterGetSlaveID (pucFrame, &usLen);
    } else {
      fprintf(stderr, "Error! Unknown register type (%s). Known types are: coils\n", reg_type);
      exit(EXIT_FAILURE);
    }
  } else {
    fprintf(stderr, "Error! No register type (-t) specified\n");
    exit(EXIT_FAILURE);
  }

  if(reg_action) {
    if(regt == MB_TYPE_COILS) {
      if(strncmp(reg_action, "read", 4) == 0) {
        int val;
        if(sscanf(reg_action, "%*s %i", &val) != 1) {
          fprintf(stderr, "Error! No number of coils specified.\n");
          exit(EXIT_FAILURE); 
        }
        if(val%8 == 0) {
          build_eMBMasterReadCoils (pucFrame, reg_start, val, &usLen);
        } else {
          fprintf(stderr, "Error! Invalid coil value (%d). Must be a multiple of 8.\n", val);
          exit(EXIT_FAILURE);
        }
      } else if(strncmp(reg_action, "writesingle", 11) == 0) {
        char buf[4];
        if(sscanf(reg_action, "%*s %3[FNO]", buf) != 1) {
          fprintf(stderr, "Error! No coil value specified (ON/OFF).\n");
          exit(EXIT_FAILURE); 
        }
        if(strcmp(buf, "ON") == 0)
          build_eMBMasterWriteCoils (pucFrame, reg_start, 0xff, &usLen);
        else
          build_eMBMasterWriteCoils (pucFrame, reg_start, 0x0, &usLen);
      } else if(strncmp(reg_action, "writemultiple", 13) == 0) {
        int no;
        char buf[4];
        if( sscanf(reg_action, "%*s %i %3[FNO]", &no, buf) != 2) {
          fprintf(stderr, "Error! Invalid value specified.\n");
          exit(EXIT_FAILURE); 
        }
        if(strcmp(buf, "ON") == 0)
          build_eMBMasterWriteMultipleCoils (pucFrame, reg_start, no, 0xff, &usLen);
        else
          build_eMBMasterWriteMultipleCoils (pucFrame, reg_start, no, 0x0, &usLen);
      } else {
        fprintf(stderr, "Error! Unknown command type (%s).\n", reg_action);
        exit(EXIT_FAILURE); 
      }
      
    } else if(regt == MB_TYPE_INPUT) {
      if(strncmp(reg_action, "read", 4) == 0) {
        int val;
        
        if(sscanf(reg_action, "%*s %i", &val) != 1) {
          fprintf(stderr, "Error! No number of registers specified.\n");
          exit(EXIT_FAILURE); 
        }
        if(build_eMBMasterReadInput (pucFrame, reg_start, val, &usLen) != MB_ENOERR) {
          fprintf(stderr, "Error! Wrong register count. Must be between 1 and 125 (0x7d)\n");
            exit(EXIT_FAILURE); 
        }
      }
    } else if(regt == MB_TYPE_HOLDING) {
      if(strncmp(reg_action, "read", 4) == 0) {
        int val;
        
        if(sscanf(reg_action, "%*s %i", &val) != 1) {
          fprintf(stderr, "Error! No number of registers specified.\n");
          exit(EXIT_FAILURE); 
        }
        if(build_eMBMasterReadHolding (pucFrame, reg_start, val, &usLen) != MB_ENOERR) {
          fprintf(stderr, "Error! Wrong register count. Must be between 1 and 125 (0x7d)\n");
          exit(EXIT_FAILURE); 
        }
        printf("%s():%d -  build_eMBMasterReadHolding(pucFramem, %d, %d, %d)\n", 
               __FUNCTION__, __LINE__,
               reg_start, val, usLen);
      } else if(strncmp(reg_action, "writesingle", 11) == 0) {
        int val;
        if( sscanf(reg_action, "%*s %i", &val) != 1) {
          fprintf(stderr, "Error! Invalid value specified.\n");
          exit(EXIT_FAILURE); 
        }
        if(build_eMBMasterWriteSingleHolding (pucFrame, reg_start, val, &usLen) != MB_ENOERR) {
          fprintf(stderr, "Error!\n");
          exit(EXIT_FAILURE); 
        }
      }
    }
  } else {
    fprintf(stderr, "Error! No command (-c) specified.\n");
    exit(EXIT_FAILURE); 
  }

  /* mode, port, baud, parity */
  eMBMasterInit(MB_RTU, ttydev, baud, parity);

  /* ? */
  eMBEnable();
  
  /* Set the address of the device in question */
  eMBSetSlaveAddress(address);
  
  /* Send the frame */
  if( (err = eMBSendFrame(pucFrame, usLen)) != MB_ENOERR) {
    printf("Error sending frame (%d).\n", err);
    exit(EXIT_FAILURE);
  }

  eMBException    eException;
  eException = MB_EX_ILLEGAL_FUNCTION;
  UCHAR ucFuncType = pucFrame[MB_PDU_FUNC_OFF];
  
  USHORT retval[255];

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
        eException = parse_eMBMasterReadCoils(pucFrame, &usLen, (void *)retval);
        printf("Reply: %04x\n", retval[0]);
        break;
     case MB_FUNC_WRITE_SINGLE_COIL:
        eException = parse_eMBMasterWriteCoils(pucFrame, &usLen, (void *)retval);
        printf("Reply: %04x\n", retval[0]);
        break;
      case MB_FUNC_READ_INPUT_REGISTER:
        eException = parse_eMBMasterReadInput(pucFrame, &usLen, (void *)retval);
        printf("Reply (%d): %04x\n", eException, retval[0]);
        break;
      case MB_FUNC_READ_HOLDING_REGISTER:
        eException = parse_eMBMasterReadHolding(pucFrame, &usLen, (void *)retval);
        printf("Reply: %04x\n", retval[0]);
        break;
      case MB_FUNC_WRITE_REGISTER:
        eException = parse_eMBMasterWriteSingleHolding(pucFrame, &usLen, (void *)retval);
        printf("Reply: %04x\n", retval[0]);
        break;
      case MB_FUNC_WRITE_MULTIPLE_COILS:
        usLen = 5;
        eException =  parse_eMBMasterWriteMultipleCoils(pucFrame, &usLen, (void *)retval);
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
  }
    
#ifdef DBG
  printf("%d : %s()\n", __LINE__, __FUNCTION__);
  {
    int j;
    for(j=0; j<usLen; j++)
      printf("%02x ", pucFrame[j]); 
    printf("\n");
  }
#endif

  eMBClose();
  
  return 0;
}
