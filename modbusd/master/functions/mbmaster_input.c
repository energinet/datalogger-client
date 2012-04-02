/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbconfig.h"
#include "mb-mastfunc.h"


/* ----------------------- Defines ------------------------------------------*/
#define MB_PDU_FUNC_READ_ADDR_OFF           ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_READ_REGCNT_OFF         ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_READ_SIZE               ( 4 )
#define MB_PDU_FUNC_READ_REGCNT_MAX         ( 0x007D )

#define MB_PDU_FUNC_READ_RSP_BYTECNT_OFF    ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_READ_VALUE_OFF          ( MB_PDU_FUNC_READ_RSP_BYTECNT_OFF + 1 )

/* ----------------------- Static functions ---------------------------------*/
eMBException    prveMBError2Exception( eMBErrorCode eErrorCode );

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode build_eMBMasterReadInput (UCHAR * pucFrame, 
                                       USHORT usRegAddress, 
                                       USHORT usRegCount, 
                                       USHORT *usLen)
{
  UCHAR          *pucFrameCur;
    
  pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
  *usLen = MB_PDU_FUNC_OFF;  

  /* Assign function code */
  pucFrameCur[MB_PDU_FUNC_OFF] = MB_FUNC_READ_INPUT_REGISTER;
  *usLen += 1;

  /* Holding register starting address */
  pucFrameCur[MB_PDU_FUNC_READ_ADDR_OFF]       = (usRegAddress >> 8)&0xff;
  pucFrameCur[MB_PDU_FUNC_READ_ADDR_OFF + 1]   = (usRegAddress&0xff);
  *usLen += 2;
  
  /* Quantity of registers to read */
  if(usRegCount == 0 || usRegCount > 0x7d)
    return MB_EINVAL;

  pucFrameCur[MB_PDU_FUNC_READ_REGCNT_OFF]    = (usRegCount >> 8)&0xff;
  pucFrameCur[MB_PDU_FUNC_READ_REGCNT_OFF +1] = (usRegCount&0xff);
  *usLen += 2;

  return MB_ENOERR;
}


eMBException parse_eMBMasterReadInput(UCHAR * pucFrame, 
                                      USHORT *usLen, 
                                      void *data)
{
  USHORT          *usResultBuf = (USHORT *)data;
  USHORT          usRegCount;

  eMBException    eStatus = MB_EX_NONE;
  eMBErrorCode    eRegStatus;
  UCHAR          *pucFrameCur;
  
  int iRegIndex = 0;

  if( *usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN ) ) {

    usRegCount = ( USHORT )( pucFrame[MB_PDU_FUNC_READ_RSP_BYTECNT_OFF]);

    //printf("usRegCount: %x *2 bytes \n", usRegCount);
    
    pucFrameCur = &pucFrame[MB_PDU_FUNC_READ_VALUE_OFF];
    
    while( usRegCount > 0 ) {
      usResultBuf[iRegIndex] = *pucFrameCur++ << 8;
      usResultBuf[iRegIndex] |= *pucFrameCur++;
      iRegIndex++;
      usRegCount--;
    }
    
    eRegStatus = MB_ENOERR;            
  } else {
    /* Can't be a valid read coil register request because the length
     * is incorrect. */
    eStatus = MB_EX_ILLEGAL_DATA_VALUE;
  }
  return eStatus;
}
