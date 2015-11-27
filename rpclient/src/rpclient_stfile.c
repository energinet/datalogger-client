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
#include "rpclient_stfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


#define STATUS_FILE "/tmp/rpclient_status"


void rpclient_stfile_write(const char *format, ...){
  
  FILE *file;
  va_list ap;

  if((file = fopen(STATUS_FILE,"w"))<0) 
        return;
  
  va_start(ap, format);
  vfprintf(file, format, ap);
  va_end(ap);

  fclose(file);

}

