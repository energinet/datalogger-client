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

#ifndef RPFILE_UTIL_H_
#define RPFILE_UTIL_H_
#include <sys/types.h>
#include <sys/stat.h>


int rpfile_md5(unsigned char * testBuffer, size_t testLen, char* obuf);

int rpfile_read(const char *path, char **buffer, size_t *size, mode_t *mode);

int rpfile_write(const char *path, char *buffer, size_t size, mode_t mode);


#endif /* RPFILE_UTIL_H_ */
