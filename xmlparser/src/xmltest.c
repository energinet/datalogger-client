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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "xml_serialize.h"

const  char *HelpText =
 "cmddbutil: util for command database... \n"
 " -i <file>       : Input file\n"
 " -o <file>       : Output file\n"
 " -s <size>       : Chunk size\n"
 " -d <level>      : Set debug level \n"
 " -h              : Help (this text)\n"
 "Christian Rostgaard, Jan 2011\n" 
 "Version: \n"
 "";



int main(int narg, char *argp[])
{
    int optc;
    FILE *input = stdin;
    FILE *output = stdout;
    int chunk_size = 4096;
    int debug_level = 0;
    char *inbuf = NULL;
    char *outbuf = NULL;
    int retval, size;
    struct xml_stack *stack = NULL;

     while ((optc = getopt(narg, argp, "i:o:s:d::h")) > 0) {
	  switch (optc){ 
	    case 'i':
	      input = fopen(optarg, "r");
	      break;
	    case 'o':
	      output = fopen(optarg, "w");
	      break;
	    case 's':
	      chunk_size = atoi(optarg);
	      break;
	    case 'd':
	      if(optarg)
		  debug_level = atoi(optarg);
	      else
		  debug_level = 1;
	      break;
	    default:
	      fprintf(stderr, "%s", HelpText);
	      exit(0);
	      break;
	  }
     }

     stack = xml_doc_parse_prep(30, 1024, debug_level);

     inbuf = malloc(chunk_size);
     outbuf = malloc(1024*1000);

     while( (size = fread(inbuf, 1, chunk_size,input))>0){
	 fprintf(stderr, "read %d: %s\n", size, inbuf);
	 retval = xml_doc_parse_step(stack, inbuf, size);
	 fprintf(stderr, "read %d, ret %d\n", size, retval);
	 if(retval <0)
	     break;
     }
     
     fprintf(stdout,"printing:\n");
     xml_doc_print(stack->doc, outbuf, 1024*1000);

     fprintf(stdout,"out:\n");
     fprintf(output, "%s\n", outbuf);

     xml_doc_parse_free(stack);

     if(input != stdin)
	 fclose(input);

     if(output != stdout)
	 fclose(stdout);
     
     free(inbuf);
     free(outbuf);


     return 0;
    
}
