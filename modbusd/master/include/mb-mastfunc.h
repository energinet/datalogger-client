#ifndef _MB_MASTERFUNC_H
#define _MB_MASTERFUNC_H



/* Coil registers */

eMBException build_eMBMasterReadCoils (UCHAR * pucFrame, 
                                       USHORT usRegAddress, 
                                       USHORT usCoilCount, 
                                       USHORT *usLen);

eMBException parse_eMBMasterReadCoils(UCHAR * pucFrame, 
                                      USHORT *usLen, 
                                      void *data);

eMBException build_eMBMasterWriteCoils (UCHAR * pucFrame, 
                                        USHORT usRegAddress, 
                                        USHORT usCoilVal, 
                                        USHORT *usLen);

eMBException parse_eMBMasterWriteCoils(UCHAR * pucFrame, 
                                       USHORT *usLen, 
                                       void *data);


eMBException build_eMBMasterWriteMultipleCoils (UCHAR * pucFrame, 
                                        USHORT usRegAddress, 
                                        USHORT usNoOutputs, 
                                        USHORT usCoilVal, 
                                        USHORT *usLen);

void build_eMBMasterGetSlaveID (UCHAR * pucFrame, USHORT *usLen);





/* Input registers */
eMBErrorCode build_eMBMasterReadInput (UCHAR * pucFrame, 
                                       USHORT usRegAddress, 
                                       USHORT usRegCount, 
                                       USHORT *usLen);

eMBException parse_eMBMasterReadInput(UCHAR * pucFrame, 
                                      USHORT *usLen, 
                                      void *data);

/* Holding registers */
eMBException parse_eMBMasterReadHolding(UCHAR * pucFrame, 
                                        USHORT *usLen, 
                                        void *data);

eMBErrorCode build_eMBMasterReadHolding (UCHAR  * pucFrame, 
                                         USHORT usRegAddress, 
                                         USHORT usRegCount, 
                                         USHORT * usLen);

eMBErrorCode build_eMBMasterWriteSingleHolding (UCHAR  * pucFrame, 
                                                USHORT usRegAddress, 
                                                USHORT usValue, 
                                                USHORT * usLen);

eMBException parse_eMBMasterWriteSingleHolding(UCHAR * pucFrame, 
                                               USHORT *usLen, 
                                               void *data);

#endif
