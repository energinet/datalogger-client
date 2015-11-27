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
#ifndef KAMCOM_H_
#define KAMCOM_H_

#include <pthread.h>

#include "kamcom_cmd.h"
#include "kamcom_types.h"



struct kamcom {
    int fd;
    int retry;
    pthread_mutex_t mutex;
};



struct kamcom *kamcom_create(const char *path, int baud_tx, int baud_rx);

int kamcom_activate(struct kamcom *kam);

int kamcom_test(struct kamcom *kam, unsigned char addr);

int kamcom_trx(struct kamcom *kam, struct kamcom_cmd *cmd);

void kamcom_delete(struct kamcom *kam);


#endif /* KAMCOM_H_ */
