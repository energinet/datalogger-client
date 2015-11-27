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
#include "kamcom.h"

#include <stdlib.h>
#include <assert.h>
#include <termio.h>
#include <termios.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <poll.h>

#include "kamcom_cmd.h"

int kamcom_tty_speed(int speed)
{
	switch(speed){
	  case 115200:
		return B115200;
	  case 38400:
		return B38400;
	  case 19200:
		return B19200;
	  case 9600:
		return B9600;
	  case 1200:
		return B1200;
	case 300:
	        return B300;
	  default:
		return speed;
	}	
	
}

void setlines(int fd, int rts, int dtr)
{

    int lines = 0;

    if (rts) lines |= TIOCM_RTS;
    if (dtr) lines |= TIOCM_DTR;
    
    ioctl(fd,TIOCMSET,&lines);

}

int kamcom_attribs(int fd, int baud_tx, int baud_rx, int cflags)
{
        struct termios tty;
	syslog(LOG_INFO, "Setting port attributes: speed %d%d %s%s%s%s", 
	       baud_tx, baud_rx, 
	       (!(cflags&(PARENB|PARODD)))?"none":"", 
	       (cflags&PARENB)?"even":"",
	       (cflags&PARODD)?"odd":"",
	       (cflags&CSTOPB)?"2 stop bits":""
	       );

	int speed_tx = kamcom_tty_speed(baud_tx);
	int speed_rx = kamcom_tty_speed(baud_rx);
	
	syslog(LOG_DEBUG, "tx %d, rx %d", speed_tx, speed_rx);

        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                syslog(LOG_ERR, "error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed_tx);
        cfsetispeed (&tty, speed_rx);
	cfmakeraw(&tty);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // ignore break signal
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD | CSTOPB);      // shut off parity and 2nd stopbyte
        tty.c_cflag |= cflags;
	//tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

	tty.c_iflag &= ~(INLCR|ICRNL);

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                syslog(LOG_ERR, "error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}


int kamcom_open(struct kamcom *kam,  const char *path, int baud_tx, int baud_rx)
{

    syslog(LOG_DEBUG, "TTY opening %s", path);

    //    kam->fd = open (path, O_RDWR | O_NOCTTY | O_SYNC);
    kam->fd = open (path, O_RDWR);

    if(kam->fd < 0){
	syslog(LOG_ERR, "error %d opening %s: %m", errno, path);
	return -errno;
    }

    syslog(LOG_DEBUG, "TTY opened %s at fd=%d", path, kam->fd);
    
    syslog(LOG_DEBUG, "TTY Setting attribs speed %d/%d", baud_tx, baud_rx);
    
    return kamcom_attribs(kam->fd, baud_tx, baud_rx, CSTOPB );

}


struct kamcom *kamcom_create(const char *path, int baud_tx, int baud_rx)
{
    int retval;
    struct kamcom *kam = malloc(sizeof(*kam));
    assert(kam);
    
   pthread_mutex_init(&kam->mutex, NULL);
   
    kam->fd = -1;    
    kam->retry = 2;
    retval =  kamcom_open(kam, path, baud_tx, baud_rx);
    
    if(retval < 0){
	syslog(LOG_DEBUG, "TTY Error opening port: %s (%d)", strerror(-retval), retval);
	free(kam);
	return NULL;
    }
        
    setlines(kam->fd, 1, 0);

    return kam;
}

int kamcom_activate(struct kamcom *kam)
{
    int i; 
    for(i = 0; i  < 9 ; i++)
	tcsendbreak(kam->fd, 500) ;

    return 0;
}


void kamcom_close(struct kamcom *kam)
{
    if(kam->fd != -1)
	close(kam->fd);
    kam->fd = -1;
}

#define BUFSIZE 1024

int
read_and_print(int fd, int sec, int usec)
{
    int         rc,l,i;
    unsigned char        buf[BUFSIZE+1];
    //fd_set      set;
    //struct timeval  tv;
    
    /*if (sec || usec) {
    FD_ZERO(&set);
    FD_SET(fd,&set);
    tv.tv_sec  = sec;

    tv.tv_usec = usec;
    if (0 == select(fd+1,&set,NULL,NULL,&tv))
        return -1;
    }*/
    
    switch (rc = read(fd,buf,BUFSIZE)) {
    case 0:
    printf("EOF");
    exit(0);
    break;
    case -1:
    perror("read");
    exit(1);
    default:
    for (l = 0; l < rc; l+= 16) {
        printf("%04x  ",l);
        for (i = l; i < l+16; i++) {
        if (i < rc)
            printf("%02x ",buf[i]);
        else
            printf("-- ");
        if ((i%4) == 3)
            printf(" ");
        }
        for (i = l; i < l+16; i++) {

        if (i < rc)
            printf("%c",isalnum(buf[i]) ? buf[i] : '.');
        }
        printf("\n");
    }
    break;
    }
    return rc;
}

void kamcom_test_print(const unsigned char *buf, int len){
    
    int i;

    for (i = 0; i < len; i++) {
	
	printf("%02x ",buf[i]);
	    
        if ((i%4) == 3)
            printf(" ");
    } 

    printf("\n");

}


/**
 * Start byte: 0x80 from master, 0x40 from meter.
 * Stop byte: 0x0D. 

 * Bit stuffing:
 * 0x80 --> 0x1B,0x7F
 * 0x40 --> 0x1B,0xBF
 * 0x0D --> 0x1B,0xF2
 * 0x06 --> 0x1B,0xF9
 * 0x1B --> 0x1B,0xE4
 */


int kamcom_link_add();

int kamcom_tx_phys(struct kamcom *kam, const unsigned char *msg, int len)
{ 

    //printf("Sending: ");
    //    kamcom_test_print(msg, len);
    //printf("\n");

    if (write(kam->fd, msg, len) != len) {
	printf("%s:\t Error writing to serialport: %s (%d)\n", __FUNCTION__, strerror(errno), errno);
	return -1;
    }


    
    return 0;
}

#define KAMCOM_ST_MAS 0x80
#define KAMCOM_ST_MET 0x40
#define KAMCOM_STOP   0x0D
#define KAMCOM_ACK    0x06
#define KAMCOM_ESC    0x1B


int kamcom_tx_link(struct kamcom *kam, const unsigned char *msg_in, int len_in)
{
    unsigned char *msg_out = malloc(sizeof(char)*(len_in*2+2));
    int ptr = 0 , i, ret;
    
    msg_out[ptr++] = KAMCOM_ST_MAS;

    for(i = 0 ; i < len_in ; i++){
	switch((unsigned char)msg_in[i]){
	case KAMCOM_ST_MAS:
	case KAMCOM_ST_MET:
	case KAMCOM_STOP:
	case KAMCOM_ACK:
	case KAMCOM_ESC:
	    msg_out[ptr++] = 0x1B;
	    msg_out[ptr++] = ~msg_in[i];
	    break;
       	default:
	    msg_out[ptr++] = msg_in[i];
	    break;
	}
    }
    
    msg_out[ptr++] = KAMCOM_STOP;
    
    ret = kamcom_tx_phys(kam, msg_out, ptr);
    
    free(msg_out);
    
    return ret;
}


int kamcom_rx_until(struct kamcom *kam, unsigned char *msg, int maxlen, const char stop, int timeout, int dbglev)
{
    struct pollfd poll_st;
    unsigned char buf[10];
    int ret, ptr = 0;

    poll_st.fd        = kam->fd;
    poll_st.events    = POLLIN;

    while(1){
	
	ret = poll(&poll_st, 1, timeout);
	
	if(ret == 0){
	    if(dbglev)fprintf(stderr, "\n");
	    syslog(LOG_WARNING, "TTY Poll timeout %d", timeout);
	    return -ETIMEDOUT;
	} else if(ret < 0){
	    syslog(LOG_ERR, "TTY Error in poll: %s (%d)", strerror(errno), errno);
	    return -errno;
	}
		
	if((ret = read(kam->fd,buf,1))<0){
	    syslog(LOG_ERR, "Read failed  %d\n", ret);
	    return -1;
	}
	if(buf[0] == stop){
	    if(dbglev)fprintf(stderr, ">%2.2x<\n", buf[0]);
	    break;
	}

	if(dbglev)fprintf(stderr, "%2.2x ", buf[0]);

	msg[ptr++] = buf[0];
    }

    if(dbglev)fprintf(stderr, "\n");

    return ptr;
    
}


int kamcom_rx_phys(struct kamcom *kam, unsigned char *msg, int maxlen)
{

    int ret;

    syslog(LOG_DEBUG,  "Reading to start.....\n");

    ret = kamcom_rx_until(kam, msg, maxlen, KAMCOM_ST_MET, 500,0);
    
    if(ret < 0){
	syslog(LOG_WARNING, "Reading to start failed: %s (%d)\n", strerror(-ret), ret);
	return ret;
    }

    syslog(LOG_DEBUG , "Reading to end.....\n");
    
    ret = kamcom_rx_until(kam, msg, maxlen, KAMCOM_STOP, 500, 0);
  
    if(ret <= 0){
	syslog(LOG_WARNING, "Reading to end failed: %s (%d)\n", strerror(-ret),ret );
	return ret;
    }

    syslog(LOG_DEBUG , "Reading ended.....\n");
    
    //   kamcom_test_print(msg, ret);
    
    return ret;
}

int kamcom_rx_link(struct kamcom *kam, unsigned char *msg_out, int maxlen){
    unsigned char msg_in[512];
    int len, i, ptr = 0;

    len =  kamcom_rx_phys(kam, msg_in, sizeof(msg_in));

    if(len <= 0){
	return len;
    }

    i = 0;
    while(i < len){
	if(msg_in[i] == KAMCOM_ESC){
	    i++;
	    msg_out[ptr++] = ~msg_in[i++];
	} else {
	    msg_out[ptr++] = msg_in[i++];
	}
    }

  
    //    kamcom_test_print(msg_out,  ptr);
    
    return ptr;
    

}







int kamcom_trx(struct kamcom *kam, struct kamcom_cmd *cmd)
{
    unsigned char buf_tx[512];
    unsigned char buf_rx[512];

    int len_tx, len_rx, retry = kam->retry, ret = 0;

    if(pthread_mutex_lock(&kam->mutex)!=0)
	return -1;
    
    len_tx =  kamcom_cmd_request(cmd,  buf_tx, sizeof(buf_tx));

    if(len_tx <= 0){
	syslog(LOG_ERR, "ERROR creating cmd buffer %d\n", len_tx);
	ret = -1;
	goto out;
    }
    
    //    kamcom_test_print(buf_tx, len_tx );

    do{
	kamcom_tx_link(kam, buf_tx, len_tx);
    } while((len_rx = kamcom_rx_link(kam, buf_rx, sizeof(buf_rx)))==-ETIMEDOUT
	    && retry-- > 0);
    
    if(len_rx <= 0){
	syslog(LOG_ERR, "ERROR reading message: %s %d", strerror(-len_rx), len_rx);
	ret = -1;
	goto out;
    }
    
        kamcom_test_print(buf_rx, len_rx );

    kamcom_cmd_responce(cmd, buf_rx,  len_rx);

 out:
    pthread_mutex_unlock(&kam->mutex);

    
    return ret;
}
 


int kamcom_test(struct kamcom *kam, unsigned char addr)
{
    unsigned char gettempdata[] = {0x01, 0x00, 0x56};

    struct kamcom_cmd *cmd;
    
    cmd = kamcom_cmd_create(addr, 0x01);

    kamcom_trx(kam, cmd);

    kamcom_cmd_delete(cmd);

    cmd = kamcom_cmd_create(addr, 0x10);
    
    kamcom_cmd_add_reqdata(cmd , gettempdata, 3);

    usleep(100000);

    kamcom_trx(kam, cmd);

    

    kamcom_cmd_delete(cmd);



    return 0;
}


void kamcom_delete(struct kamcom *kam)
{
    kamcom_close(kam);
    pthread_mutex_destroy(&kam->mutex);
    free(kam);

}

