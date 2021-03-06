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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <syslog.h>
#include <asocket_trx.h>
#include <asocket_client.h>
#include <qDecoder.h>
#include <errno.h>
#define DEFAULT_PORT 6523

#include "siteutil.h"

int graf_print_graf(const char *flashvars)
{
    printf("<embed width=\"750px\" height=\"500px\" src=\"../flashchart.swf\""
	   " pluginspage=\"http://www.adobe.com/go/getflashplayer\" "
	   " flashvars=\"%s\">"
	   " </embed>\n", flashvars);
    return 0;
}


int graf_print_all(void)
{
#ifndef SDVP
    return graf_print_graf("");
#else
    graf_print_graf("show=unit:W,kWh&unit=W&unitbox=W,kWh");
    graf_print_graf("show=unit:°C");
    graf_print_graf("show=unit:l%2Fmin,l&unitbox=l%2Fmin,l");

	return 0;
#endif
}

int main(int argc, char *argv[])
{

  struct sitereq site;

  openlog("grafer.cgi", LOG_PID, LOG_DAEMON);

  siteutil_init(&site, "Kurver", "graf");
  
  syslog(LOG_NOTICE, "Reading configuration file...");

  siteutil_top(&site, 0);

  graf_print_all();

  siteutil_bot(&site);

  siteutil_deinit(&site);

  syslog(LOG_NOTICE, "Done");
  return EXIT_SUCCESS;


}
