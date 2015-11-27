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
#include "siteutil.h"




int main(int argc, char *argv[])
{

    struct sitereq site;
	
    openlog("export.cgi", LOG_PID, LOG_DAEMON/*|LOG_PERROR*/);

	syslog(LOG_ERR, "Done");

    siteutil_init(&site, "Export", "export");
    
    syslog(LOG_DEBUG, "Reading configuration file...");
    
    siteutil_top(&site, 0);

	printf("<a href=\"/cgi-bin/dbfile.cgi\">Download file</a>");

    siteutil_bot(&site);

    siteutil_deinit(&site);

    syslog(LOG_DEBUG, "Done");
    return EXIT_SUCCESS;

}
