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
#include "kamcom_cmd.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <math.h>

#include "kamcom_types.h"


int kamcom_cmd_add_reg(struct kamcom_cmd *cmd, short reg);


unsigned short crc16(const unsigned char *buf, int sz)
{
        unsigned short crc = 0;
        while (--sz >= 0) {
                int i;
                crc ^= (unsigned short) *buf++ << 8;
                for (i = 0; i < 8; i++)
                        if (crc & 0x8000)
                                crc = crc << 1 ^ 0x1021;
                        else
                                crc <<= 1;
        }
        return crc;
}


struct kamcom_cmd *kamcom_cmd_create(unsigned char addr, unsigned char cmd)
{
    struct kamcom_cmd *new = malloc(sizeof(*new));
    assert(new);

    new->addr = addr;;
    new->cmd  = cmd;
    new->request = NULL;
    new->request_len = 0;
    new->responce = NULL;
    new->responce_len = 0;
    
    return new;
    
}




struct kamcom_cmd *kamcom_cmd_create_getreg(unsigned char addr, unsigned short reg)
{
    struct kamcom_cmd *new = kamcom_cmd_create(addr, 0x10);
    
    kamcom_cmd_add_reg(new, reg);
    
    return new;
    
}





void kamcom_cmd_delete(struct kamcom_cmd *cmd)
{
    free(cmd->request);
    free(cmd->responce);
    free(cmd);
}


int kamcom_cmd_add_reqdata(struct kamcom_cmd *cmd, const unsigned char *buf, int len)
{
    free(cmd->request);
    
    cmd->request = malloc(len);
    assert(cmd->request);

    cmd->request_len = len;
    memcpy(cmd->request, buf, len);

    return 0;
    
}

int kamcom_cmd_add_reg(struct kamcom_cmd *cmd, short reg)
{
    unsigned char count = 1;
    int len = 1 + count * 2;

    free(cmd->request);
    
    cmd->request = malloc(len);
    assert(cmd->request);

    cmd->request_len = len;
    cmd->request[0] = count;
    cmd->request[1] = (reg>>8)&0xff;
    cmd->request[2] = reg&0xff;

    return 0;
}


/**
 * Create a unstuffed and untermiated package 
 */
int kamcom_cmd_request(struct kamcom_cmd *cmd, unsigned char *buf, int maxlen)
{
    int ptr = 0;
    
    if(maxlen < (cmd->request_len + 4) /* adr + cmd + crc */)
	return -1;

    buf[ptr++] = cmd->addr;
    buf[ptr++] = cmd->cmd;
    
    memcpy(buf+ptr, cmd->request, cmd->request_len );

    ptr +=  cmd->request_len;
    
    unsigned short crc = crc16(buf, ptr);

    //    fprintf(stderr, "CRC %X\n",  crc);
    
    buf[ptr++] = (crc>>8)&0xff;
    buf[ptr++] = crc&0xff;

    return ptr;	
}


/**
 * Parses a unstuffed and untermiated package 
 */
int kamcom_cmd_responce(struct kamcom_cmd *cmd, const unsigned char *buf, int len)
{
    if(len <= 4 )
	return 0;

    unsigned short crc = crc16(buf, len-2);
    unsigned short crc_msg = buf[len-1] + (buf[len-2]<<8);

    //    printf("crc %x , msg %x\n", crc, crc_msg);

    if(crc != crc_msg){
	syslog(LOG_ERR, "CRC error");
	return -1;
    }
	
    if(cmd->addr != buf[0]){
	syslog(LOG_ERR, "Address mismatch");
	return -1;
    }

    if(cmd->cmd != buf[1]){
	syslog(LOG_ERR, "Command mismatch");
	return -1;
    }
    
    cmd->responce_len = len-4 /* addr, crc */;
    
    if(cmd->responce_len<=0)
	return 0;

    cmd->responce = malloc(cmd->responce_len);
    assert(cmd->responce);

    memcpy(cmd->responce, buf+2, cmd->responce_len);
    
    return 0;
}

int kamcom_cmd_rd_regi(struct kamcom_cmd *cmd, short reg, int *value, unsigned char *unit_)
{
    unsigned char *resp = cmd->responce;
    int ptr = 0;

    while(ptr < cmd->responce_len){
	short reg_in = (resp[ptr++]<<8);
	reg_in |= resp[ptr++];

	syslog(LOG_DEBUG, "Reg 0x%x\n", reg_in);
	unsigned char unit = resp[ptr++];

	syslog(LOG_DEBUG, "Unit %s (0x%x)\n", kamcom_types_unit(unit), unit);

	unsigned char size = resp[ptr++];

	syslog(LOG_DEBUG, "Size 0x%x\n", size);

	unsigned char siex = resp[ptr++];

	syslog(LOG_DEBUG, "Sign/Exp  0x%x\n", siex);

	if(reg == reg_in){
	    *value =  kamcom_types_int(siex,  size, resp + ptr);
	    syslog(LOG_INFO, "Value  ,%d %s\n", *value,  kamcom_types_unit(unit));
	    if(unit_)
		*unit_ = unit;
	    return 0;
	}

	ptr += size;
	
    }
    
    return -1 ;
}

int kamcom_cmd_rd_regf(struct kamcom_cmd *cmd, short reg, float *value, unsigned char *unit_)
{
    unsigned char *resp = cmd->responce;
    int ptr = 0;

    while(ptr < cmd->responce_len){
	short reg_in = (resp[ptr++]<<8);
	reg_in |= resp[ptr++];

	syslog(LOG_DEBUG, "Reg 0x%x\n", reg_in);
	unsigned char unit = resp[ptr++];

	syslog(LOG_DEBUG, "Unit %s (0x%x)\n", kamcom_types_unit(unit), unit);
	    
	unsigned char size = resp[ptr++];

	syslog(LOG_DEBUG, "Size 0x%x\n", size);

	unsigned char siex = resp[ptr++];

	syslog(LOG_DEBUG, "Sign/Exp  0x%x\n", siex);

	if(reg == reg_in){
	    *value =  kamcom_types_number(siex,  size, resp + ptr);
	    syslog(LOG_INFO, "Value  ,%f %s\n", *value,  kamcom_types_unit(unit));
	    if(unit_)
		*unit_ = unit;
	    return 0;
	}

	ptr += size;
	
    }
    
    return -1 ;
}

