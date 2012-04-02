/*
 * FreeModbus Libary: Linux Port
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
 * File: $Id: portserial.c,v 1.3 2006/10/12 08:35:34 wolti Exp $
 */

/* ----------------------- Standard includes --------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbconfig.h"

/* ----------------------- Defines  -----------------------------------------*/
#if MB_ASCII_ENABLED == 1
#define BUF_SIZE    513         /* must hold a complete ASCII frame. */
#else
#define BUF_SIZE    256         /* must hold a complete RTU frame. */
#endif

#define DBG
#define RS485_CANCEL_ECHO
//#undef RS485_CANCEL_ECHO
#define RS485_REPLY_TIMEOUT_MS 1


/* ----------------------- Static variables ---------------------------------*/
static int      iSerialFd = -1;
static BOOL     bRxEnabled;
static BOOL     bTxEnabled;

static ULONG    ulTimeoutMs;
static UCHAR    ucBuffer[BUF_SIZE];
static int      uiRxBufferPos;
static int      uiTxBufferPos;

static struct termios xOldTIO;

enum eReply{EXPECT_ECHO, EXPECT_REPLY};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static ULONG   ulReplyTimeout = RS485_REPLY_TIMEOUT_MS;
static ULONG   ulEchoBytes = 0;
static int echo = EXPECT_ECHO;

static BOOL bEchoCancel = TRUE;


/* ----------------------- Function prototypes ------------------------------*/
static BOOL     prvbMBPortSerialRead( UCHAR * pucBuffer, USHORT usNBytes, USHORT * usNBytesRead );
static BOOL     prvbMBPortSerialWrite( UCHAR * pucBuffer, USHORT usNBytes );

/* ----------------------- Begin implementation -----------------------------*/


void
vMBPortSerialEchoCancel( BOOL bEnableEcho )
{
    bEchoCancel = bEnableEcho;
}

BOOL bPortSerialEchoTest(  )
{
  int timeout = 30;
  ssize_t  res = 0;
  UCHAR   ucBufferW[] = "LIAB";
  UCHAR   ucBufferR[64];
  USHORT   usNBytes = sizeof(ucBufferW);

//  fprintf(stderr, "bPortSerialEchoTest testing echo\n");

  if( ( res = write( iSerialFd, ucBufferW, usNBytes) ) == -1 ){
      fprintf(stderr, "bPortSerialEchoTest return after WRITE ret %d\n", res);
      return FALSE;
  }

  //fprintf(stderr, "bPortSerialEchoTest messgae sent %s\n",ucBufferW);
  
  res = 0;
  do{
      usleep(1000);
      if( ( res += read( iSerialFd, ucBufferR, usNBytes) ) < 0){
	  fprintf(stderr, "bPortSerialEchoTest return after READ ret %d\n", res);
      return FALSE;
      }
      
  }while((res<usNBytes) && (timeout-- > 0));

//  fprintf(stderr, "bPortSerialEchoTest messgae received %s %d\n",ucBufferR, res);

  if(memcmp(ucBufferR, ucBufferW, usNBytes)==0){
      fprintf(stderr, "bPortSerialEchoTest return TRUE \n");
      return TRUE;
  }

  fprintf(stderr, "bPortSerialEchoTest return FALSE \n");
  return FALSE;
}


void
vMBPortSerialEnable( BOOL bEnableRx, BOOL bEnableTx )
{
    /* it is not allowed that both receiver and transmitter are enabled. */
    assert( !bEnableRx || !bEnableTx );

    if( bEnableRx )
    {
        ( void )tcflush( iSerialFd, TCIFLUSH );
        uiRxBufferPos = 0;
        bRxEnabled = TRUE;
    }
    else
    {
        bRxEnabled = FALSE;
    }
    if( bEnableTx )
    {
        bTxEnabled = TRUE;
        uiTxBufferPos = 0;
    }
    else
    {
        bTxEnabled = FALSE;
    }
}

BOOL
xMBPortSerialInit( UCHAR *ucPort, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
    CHAR            szDevice[32];
    BOOL            bStatus = TRUE;

    struct termios  xNewTIO;
    speed_t         xNewSpeed;

    snprintf( szDevice, 32, "%s", ucPort );

    if( ( iSerialFd = open( szDevice, O_RDWR | O_NOCTTY ) ) < 0 )
    {
        vMBPortLog( MB_LOG_ERROR, "SER-INIT", "Can't open serial port %s: %s\n", szDevice,
                    strerror( errno ) );
    }
    else if( tcgetattr( iSerialFd, &xOldTIO ) != 0 )
    {
        vMBPortLog( MB_LOG_ERROR, "SER-INIT", "Can't get settings from port %s: %s\n", szDevice,
                    strerror( errno ) );
    }
    else
    {
        bzero( &xNewTIO, sizeof( struct termios ) );

        xNewTIO.c_iflag |= IGNBRK | INPCK;
        xNewTIO.c_cflag |= CREAD | CLOCAL;
        switch ( eParity )
        {
        case MB_PAR_NONE:
            break;
        case MB_PAR_EVEN:
            xNewTIO.c_cflag |= PARENB;
            break;
        case MB_PAR_ODD:
            xNewTIO.c_cflag |= PARENB | PARODD;
            break;
        default:
            bStatus = FALSE;
        }
        switch ( ucDataBits )
        {
        case 8:
            xNewTIO.c_cflag |= CS8;
            break;
        case 7:
            xNewTIO.c_cflag |= CS7;
            break;
        default:
            bStatus = FALSE;
        }
        switch ( ulBaudRate )
        {
        case 9600:
            xNewSpeed = B9600;
            break;
        case 19200:
            xNewSpeed = B19200;
            break;
        case 38400:
            xNewSpeed = B38400;
            break;
        case 57600:
            xNewSpeed = B57600;
            break;
        case 115200:
            xNewSpeed = B115200;
            break;
        default:
            bStatus = FALSE;
        }
        if( bStatus )
        {
            if( cfsetispeed( &xNewTIO, xNewSpeed ) != 0 )
            {
                vMBPortLog( MB_LOG_ERROR, "SER-INIT", "Can't set baud rate %ld for port %s: %s\n",
                            ulBaudRate, strerror( errno ) );
            }
            else if( cfsetospeed( &xNewTIO, xNewSpeed ) != 0 )
            {
                vMBPortLog( MB_LOG_ERROR, "SER-INIT", "Can't set baud rate %ld for port %s: %s\n",
                            ulBaudRate, szDevice, strerror( errno ) );
            }
            else if( tcsetattr( iSerialFd, TCSANOW, &xNewTIO ) != 0 )
            {
                vMBPortLog( MB_LOG_ERROR, "SER-INIT", "Can't set settings for port %s: %s\n",
                            szDevice, strerror( errno ) );
            }
            else
            {
                vMBPortSerialEnable( FALSE, FALSE );
                bStatus = TRUE;
            }
        }
    }

    vMBPortSerialEchoCancel( bPortSerialEchoTest(  ) );
    
    return bStatus;
}

BOOL
xMBPortSerialSetTimeout( ULONG ulNewTimeoutMs )
{
    if( ulNewTimeoutMs > 0 )
    {
        ulTimeoutMs = ulNewTimeoutMs;
    }
    else
    {
        ulTimeoutMs = 1;
    }
    return TRUE;
}

void
vMBPortClose( void )
{
    if( iSerialFd != -1 )
    {
        ( void )tcsetattr( iSerialFd, TCSANOW, &xOldTIO );
        ( void )close( iSerialFd );
        iSerialFd = -1;
    }
}



BOOL prvbMBPortSerialRead( UCHAR * pucBuffer, USHORT usNBytes, USHORT * usNBytesRead )
{
  BOOL            bResult = TRUE;
  ssize_t         res;
  fd_set          rfds;
  struct timeval  tv;

  tv.tv_sec = 0;
  tv.tv_usec = 10000;
  FD_ZERO( &rfds );
  FD_SET( iSerialFd, &rfds );

  pthread_mutex_lock( &mutex );

  /* Wait until character received or timeout. Recover in case of an
   * interrupted read system call. */
  do {
    /* Check for serial port events */
    if( select( iSerialFd + 1, &rfds, NULL, NULL, &tv ) == -1 ) {
      printf("Select error: %s\n", strerror(errno));
      if( errno != EINTR ) {
        bResult = FALSE;
      }
    } else if( FD_ISSET( iSerialFd, &rfds ) ) {
      if( ( res = read( iSerialFd, pucBuffer, (ulEchoBytes > 0) ? ulEchoBytes : usNBytes /* usNBytes */) ) == -1 ) {
        printf("Read error: %s\n", strerror(errno));
        bResult = FALSE;
      } else {

#ifdef DBG
        printf("%d : %s -- expect %ld bytes    got %d bytes (%s)\n", __LINE__,  __FUNCTION__, 
               (ulEchoBytes > 0) ? ulEchoBytes : usNBytes , res,
               (echo == EXPECT_ECHO) ? "local echo" : "slave");
        {
          int j;
          for(j=0; j<res; j++)
            printf("%02x ", pucBuffer[j]); 
          printf("\n");
        }
#endif
        printf("%s():%d - read: %d\n", __FUNCTION__, __LINE__, res);
        *usNBytesRead = ( USHORT ) res;
          
//#ifndef RS485_CANCEL_ECHO
	if(!bEchoCancel){
	  break;
	}
//#else
	if(echo == EXPECT_ECHO) {
	  ulEchoBytes -= res;
	  *usNBytesRead = 0;
          if(ulEchoBytes == 0) {
	    echo = EXPECT_REPLY;
	    break;
          } else {
	    printf("looping again for rest of echo\n");
	    //exit(0);
	    //break;
	    continue;
          } 
        } else {
	  /* Reply received. Reset expectation */
          ulReplyTimeout = RS485_REPLY_TIMEOUT_MS;
          echo = EXPECT_ECHO;
          break;
        }        
	
//#endif
      }
   
  } else {
    *usNBytesRead = 0;
    break;
  }
} while( bResult == TRUE );
  
  
  if(ulReplyTimeout <= 0) {
    bResult = FALSE;
    echo = EXPECT_ECHO;
    ulReplyTimeout = RS485_REPLY_TIMEOUT_MS;
    *usNBytesRead = 0;
    memset(pucBuffer, 0, usNBytes);
    errno = -ETIME;
    //printf("Slave reply timeout!\n");
  } else if (echo == EXPECT_REPLY){
    ulReplyTimeout--;
  }
  
  pthread_mutex_unlock( &mutex );
  
  fflush(stdout);
  return bResult;
}


BOOL
prvbMBPortSerialWrite( UCHAR * pucBuffer, USHORT usNBytes )
{
    ssize_t         res;
    size_t          left = ( size_t ) usNBytes;
    size_t          done = 0;

    ulEchoBytes = usNBytes;
#ifdef DBG
  printf("\n%d : %s()\n", __LINE__, __FUNCTION__);
  {
    int j;
    for(j=0; j<10; j++)
      printf("%02x ", pucBuffer[j]); 
    printf("\n");
  }
#endif

  pthread_mutex_lock( &mutex );

    while( left > 0 )
    {
        if( ( res = write( iSerialFd, pucBuffer + done, left ) ) == -1 )
        {
            if( errno != EINTR )
            {
                break;
            }
            /* call write again because of interrupted system call. */
            continue;
        }
        done += res;
        left -= res;
    }

    pthread_mutex_unlock( &mutex );

    return left == 0 ? TRUE : FALSE;
}

#define TMVAL 10
BOOL
xMBPortSerialPoll(  )
{
    BOOL            bStatus = TRUE;
    USHORT          usBytesRead;
    int             i;
    static int timeout = TMVAL;

    while( bRxEnabled )
    {
        if( prvbMBPortSerialRead( &ucBuffer[0], BUF_SIZE, &usBytesRead ) )
        {

            if( usBytesRead == 0 )
            {              
              timeout--;
#ifdef DBG
              printf("%s() - read timeout (%d)\n", __FUNCTION__, timeout);
#endif    
              if(timeout < 0) {
                timeout = TMVAL;
                bStatus = FALSE;
              }

              /* timeout with no bytes. */              
              break;
            }
            else if( usBytesRead > 0 )
            {
              printf("%s():%d - Bytes: %d\n", __FUNCTION__, __LINE__, usBytesRead);
                for( i = 0; i < usBytesRead; i++ )
                {
                    /* Call the modbus stack and let him fill the buffers. */
                    ( void )pxMBFrameCBByteReceived(  );
                }
                uiRxBufferPos = 0;
            }
            /* Reset timeout */
            timeout = TMVAL;
        }
        else
        {
          printf("SER-POLL", "read failed on serial device: %s\n",
                 strerror( errno ) );
            bStatus = FALSE;
            break;
        }
    }
    if( bTxEnabled )
    {
        while( bTxEnabled )
        {
            ( void )pxMBFrameCBTransmitterEmpty(  );
            /* Call the modbus stack to let him fill the buffer. */
        }
        if( !prvbMBPortSerialWrite( &ucBuffer[0], uiTxBufferPos ) )
        {
          printf("SER-POLL", "write failed on serial device: %s\n",
                 strerror( errno ) );            
            bStatus = FALSE;
        }
    }
    fflush(stdout);
    return bStatus;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
    assert( uiTxBufferPos < BUF_SIZE );
    ucBuffer[uiTxBufferPos] = ucByte;
    uiTxBufferPos++;
    return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
    assert( uiRxBufferPos < BUF_SIZE );
    *pucByte = ucBuffer[uiRxBufferPos];
    uiRxBufferPos++;
    return TRUE;
}
