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

void build_eMBMasterGetSlaveID (UCHAR * pucFrame, USHORT *usLen)
{
  UCHAR          *pucFrameCur;
  
  pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
  *usLen = MB_PDU_FUNC_OFF;  

  pucFrameCur[MB_PDU_FUNC_OFF] = MB_FUNC_OTHER_REPORT_SLAVEID;
  *usLen += 1;
}
