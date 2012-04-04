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

