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
#ifndef KAMCOM_CMD_H_
#define KAMCOM_CMD_H_


struct kamcom_cmd {
    unsigned char addr;
    unsigned char cmd;
    unsigned char *request;
    int request_len;
    unsigned char *responce;
    int responce_len;
};


unsigned short crc16(const unsigned char *buf, int sz);


struct kamcom_cmd *kamcom_cmd_create(unsigned char addr, unsigned char cmd);
struct kamcom_cmd *kamcom_cmd_create_getreg(unsigned char addr, unsigned short reg);



void kamcom_cmd_delete(struct kamcom_cmd *cmd);

int kamcom_cmd_add_reqdata(struct kamcom_cmd *cmd, const unsigned char *buf, int len);


int kamcom_cmd_request(struct kamcom_cmd *cmd, unsigned char *buf, int maxlen);

int kamcom_cmd_responce(struct kamcom_cmd *cmd, const unsigned char *buf, int len);



int kamcom_cmd_rd_regf(struct kamcom_cmd *cmd, short reg, float *value, unsigned char *unit);
int kamcom_cmd_rd_regi(struct kamcom_cmd *cmd, short reg, int *value, unsigned char *unit);


#endif
