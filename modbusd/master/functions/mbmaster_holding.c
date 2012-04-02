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


#define MB_PDU_FUNC_READ_ADDR_OFF               ( MB_PDU_DATA_OFF + 0 )
#define MB_PDU_FUNC_READ_REGCNT_OFF             ( MB_PDU_DATA_OFF + 2 )

#define MB_PDU_FUNC_READ_BYTECNT_OFF            ( MB_PDU_DATA_OFF + 0 )
#define MB_PDU_FUNC_READ_VALUE_OFF              ( MB_PDU_DATA_OFF + 1 )

#define MB_PDU_FUNC_WRITE_ADDR_OFF              ( MB_PDU_DATA_OFF + 0 )
#define MB_PDU_FUNC_WRITE_VALUE_OFF             ( MB_PDU_DATA_OFF + 2 )

#define MB_PDU_FUNC_READ_SIZE                   ( 4 )

eMBErrorCode build_eMBMasterReadHolding (UCHAR  * pucFrame, 
                                         USHORT usRegAddress, 
                                         USHORT usRegCount, 
                                         USHORT * usLen)
{
  UCHAR          *pucFrameCur;

  pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
  *usLen = MB_PDU_FUNC_OFF;  

  /* Assign function code */
  pucFrameCur[MB_PDU_FUNC_OFF] = MB_FUNC_READ_HOLDING_REGISTER;
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


eMBException parse_eMBMasterReadHolding(UCHAR * pucFrame, 
                                        USHORT *usLen, 
                                        void *data)
{
  USHORT          *usResultBuf = (USHORT *)data;
  USHORT          usByteCount;

  eMBException    eStatus = MB_EX_NONE;
  eMBErrorCode    eRegStatus;
  UCHAR          *pucFrameCur;
  
  int iRegIndex = 0;

  /*
  printf("%d : %s()\n", __LINE__, __FUNCTION__);
  {
    int j;
    for(j=0; j<10; j++)
      printf("%02x ", pucFrame[j]); 
    printf("\n");
  }
  */
  if( *usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN ) ) {

    usByteCount = ( USHORT )( pucFrame[MB_PDU_FUNC_READ_ADDR_OFF]);
    
    pucFrameCur = &pucFrame[MB_PDU_FUNC_READ_VALUE_OFF];
    
    while( usByteCount > 0 ) {
      usResultBuf[iRegIndex] = *pucFrameCur++ << 8;
      usResultBuf[iRegIndex] |= *pucFrameCur++;
      iRegIndex++;
      usByteCount-=2;
    }
    
    eRegStatus = MB_ENOERR;            
  } else {
    /* Can't be a valid read coil register request because the length
     * is incorrect. */
    eStatus = MB_EX_ILLEGAL_DATA_VALUE;
  }
  return eStatus;
}



eMBErrorCode build_eMBMasterWriteSingleHolding (UCHAR  * pucFrame, 
                                                USHORT usRegAddress, 
                                                USHORT usValue, 
                                                USHORT * usLen)
{
  UCHAR          *pucFrameCur;

  pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
  *usLen = MB_PDU_FUNC_OFF;  

  /* Assign function code */
  pucFrameCur[MB_PDU_FUNC_OFF] = MB_FUNC_WRITE_REGISTER;
  *usLen += 1;

  /* Holding register starting address */
  pucFrameCur[MB_PDU_FUNC_WRITE_ADDR_OFF]       = (usRegAddress >> 8)&0xff;
  pucFrameCur[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]   = (usRegAddress&0xff);
  *usLen += 2;
  
  /* Value */
  pucFrameCur[MB_PDU_FUNC_WRITE_VALUE_OFF]    = (usValue >> 8)&0xff;
  pucFrameCur[MB_PDU_FUNC_WRITE_VALUE_OFF +1] = (usValue&0xff);
  *usLen += 2;

  /*
printf("%d : %s()\n", __LINE__, __FUNCTION__);
  {
    int j;
    for(j=0; j<10; j++)
      printf("%02x ", pucFrame[j]); 
    printf("\n");
  }
  */
  return MB_ENOERR;
}


eMBException parse_eMBMasterWriteSingleHolding(UCHAR * pucFrame, 
                                               USHORT *usLen, 
                                               void *data)
{
  USHORT          *usResultBuf = (USHORT *)data;
  USHORT          usRegAddr;
  USHORT          usRegVal;

  eMBException    eStatus = MB_EX_NONE;
  eMBErrorCode    eRegStatus;
  

  if( *usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN ) ) {
    
    usRegAddr  = pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8;
    usRegAddr |= pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF +1];

    usRegVal   = pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] << 8;
    usRegVal  |= pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF +1];   
    
    usResultBuf[0] = usRegVal;

    /*
    printf("%d : %s()\n", __LINE__, __FUNCTION__);
  {
    int j;
    for(j=0; j<10; j++)
      printf("%02x ", pucFrame[j]); 
    printf("\n");
  }
    */

    eRegStatus = MB_ENOERR;            
  } else {
    /* Can't be a valid read coil register request because the length
     * is incorrect. */
    eStatus = MB_EX_ILLEGAL_DATA_VALUE;
  }
  return eStatus;
}
