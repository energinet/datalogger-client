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
#define MB_PDU_FUNC_READ_ADDR_OFF           ( MB_PDU_DATA_OFF + 0 )
#define MB_PDU_FUNC_READ_COILCNT_OFF        ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_READ_SIZE               ( 4 )
#define MB_PDU_FUNC_READ_COILCNT_MAX        ( 0x07D0 )

#define MB_PDU_FUNC_WRITE_ADDR_OFF          ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_WRITE_VALUE_OFF         ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_WRITE_SIZE              ( 4 )


#define MB_PDU_FUNC_BYTE_COUNT_OFF ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_STATUS_OFF ( MB_PDU_DATA_OFF + 1 )

#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF      ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF   ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF   ( MB_PDU_DATA_OFF + 4 )
#define MB_PDU_FUNC_WRITE_MUL_VALUES_OFF    ( MB_PDU_DATA_OFF + 5 )
#define MB_PDU_FUNC_WRITE_MUL_SIZE_MIN      ( 5 )
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_MAX   ( 0x07B0 )

eMBException    prveMBError2Exception( eMBErrorCode eErrorCode );

eMBException build_eMBMasterReadCoils (UCHAR * pucFrame, 
                                       USHORT usRegAddress, 
                                       USHORT usCoilCount, 
                                       USHORT *usLen)
{
  UCHAR          *pucFrameCur;
  eMBException    eStatus = MB_EX_NONE;

  pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
  *usLen = MB_PDU_FUNC_OFF;  

  /* Assign function code */
  pucFrameCur[MB_PDU_FUNC_OFF] = MB_FUNC_READ_COILS;
  *usLen += 1;

  /* Coil starting address */
  pucFrameCur[MB_PDU_FUNC_READ_ADDR_OFF]       = (usRegAddress >> 8)&0xff;
  pucFrameCur[MB_PDU_FUNC_READ_ADDR_OFF + 1]   = (usRegAddress&0xff);
  *usLen += 2;

  /* Quantity of coils to read */
  if(usCoilCount == 0 || usCoilCount > 0x7d0)
    return MB_EINVAL;
  
  pucFrameCur[MB_PDU_FUNC_READ_COILCNT_OFF]    = (usCoilCount >> 8)&0xff;
  pucFrameCur[MB_PDU_FUNC_READ_COILCNT_OFF +1] = (usCoilCount&0xff);
  *usLen += 2;

  return eStatus;
}


eMBException parse_eMBMasterReadCoils(UCHAR * pucFrame, 
                                      USHORT *usLen, 
                                      void *data)
{
  UCHAR           ucByteCount;
  UCHAR          *retval = (UCHAR *)data;

  eMBException    eStatus = MB_EX_NONE;
  eMBErrorCode    eRegStatus;
  int i;
 
  if( *usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN ) )
    {
      ucByteCount= ( USHORT )( pucFrame[MB_PDU_FUNC_BYTE_COUNT_OFF]);

      for(i=0; i<ucByteCount; i++)
        retval[i] = pucFrame[MB_PDU_FUNC_STATUS_OFF+i];
      
      eRegStatus = MB_ENOERR;            
    }
  else
    {
      /* Can't be a valid read coil register request because the length
       * is incorrect. */
      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }

  return eStatus;
}

eMBException build_eMBMasterWriteCoils (UCHAR * pucFrame, 
                                        USHORT usRegAddress, 
                                        USHORT usCoilVal, 
                                        USHORT *usLen)
{
  UCHAR          *pucFrameCur;
  eMBException    eStatus = MB_EX_NONE;

  pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
  *usLen = MB_PDU_FUNC_OFF;  

  pucFrameCur[MB_PDU_FUNC_OFF] = MB_FUNC_WRITE_SINGLE_COIL;
  *usLen += 1;

  pucFrameCur[MB_PDU_FUNC_WRITE_ADDR_OFF]       = (usRegAddress >> 8)&0xff;
  pucFrameCur[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]   = (usRegAddress&0xff);
  *usLen += 2;
  
  pucFrameCur[MB_PDU_FUNC_WRITE_VALUE_OFF] = usCoilVal ? 0xff : 0x00;
  pucFrameCur[MB_PDU_FUNC_WRITE_VALUE_OFF +1 ] = 0x00;
  *usLen += 2;

  return eStatus;
}

eMBException parse_eMBMasterWriteCoils(UCHAR * pucFrame, 
                                       USHORT *usLen, 
                                       void *data)
{
  UCHAR           ucByteCount;
  USHORT          *usResultBuf = (USHORT *)data;

  eMBException    eStatus = MB_EX_NONE;
  eMBErrorCode    eRegStatus;
  UCHAR          *pucFrameCur;
 
  if( *usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN ) )
    {
      ucByteCount= ( USHORT )( pucFrame[MB_PDU_FUNC_BYTE_COUNT_OFF]);

      pucFrameCur = &pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF];

      usResultBuf[0] = *pucFrameCur << 8;
      usResultBuf[1] |= *pucFrameCur++;
      
      eRegStatus = MB_ENOERR;            
    }
  else
    {
      /* Can't be a valid read coil register request because the length
       * is incorrect. */
      eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
  return eStatus;
}

eMBException build_eMBMasterWriteMultipleCoils (UCHAR * pucFrame, USHORT usRegAddress, USHORT usNoOutputs, USHORT usCoilVal, USHORT *usLen)
{
  UCHAR          *pucFrameCur;
  UCHAR           ucByteCount;
  int             i,j;
  eMBException    eStatus = MB_EX_NONE;
  
  pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
  *usLen = MB_PDU_FUNC_OFF;  

  pucFrameCur[MB_PDU_FUNC_OFF] = MB_FUNC_WRITE_MULTIPLE_COILS;
  *usLen += 1;

  pucFrameCur[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF]       = (usRegAddress >> 8)&0xff;
  pucFrameCur[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]   = (usRegAddress&0xff);
  *usLen += 2;

  pucFrameCur[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF]       = (usNoOutputs >> 8)&0xff;
  pucFrameCur[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1]   = (usNoOutputs&0xff);
  *usLen += 2;
  
  ucByteCount = ( UCHAR ) (usNoOutputs/8 +  (usNoOutputs%8 ? 1 : 0));

  pucFrameCur[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF]  = ucByteCount;
  *usLen += 1;
  
  for(i=0; i<ucByteCount; i++) {
    pucFrameCur[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF + i] = 0;
    for(j=0; j<8; j++) {
      if(j*(i+1) < usNoOutputs)
        pucFrameCur[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF + i] |= ((usCoilVal ? 0x1 : 0x0) << j);
    }
    *usLen += 1;
  }


#ifdef DBG
  printf("%d : %s()\n", __LINE__, __FUNCTION__);
  {
    int i;
    for(i=0; i<*usLen; i++)
      printf("%02x ", pucFrameCur[i]);

    printf("\n");
  }
#endif

  return eStatus;
}

eMBException parse_eMBMasterWriteMultipleCoils(UCHAR * pucFrame, USHORT *usLen, void *data)
{
    USHORT          usRegAddress;
    USHORT          usRegCount;

    eMBException    eStatus = MB_EX_NONE;

    if( *usLen == ( MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN ) )
    {
        usRegAddress = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8 );
        usRegAddress |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1] );

        usRegCount = ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF] << 8 );
        usRegCount |= ( USHORT )( pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1] );
        
        if(usRegCount%8 == 0)
        {
          eStatus = MB_EX_NONE;
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else
    {
        /* Can't be a valid write coil register request because the length
         * is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return eStatus;
}

