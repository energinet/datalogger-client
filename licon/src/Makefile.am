VERSIONINC = version.h

#lib_LTLIBRARIES  = libasocket.la #libliconsk.la
bin_PROGRAMS = liabconnect liconutil
BUILT_SOURCES = version.h

# Main application
liabconnect_SOURCES  = liabconnect.c licon_util.c licon_log.c licon_if.c licon_conf.c licon_app.c licon_server.c licon_client.c licon_check.c
liconutil_SOURCES = liconutil.c licon_client.c
#liabconnect_LDADD   =  -lasocket
#libasocket_la_SOURCES = asocket_server.c asocket_client.c
#libliconsk_la_SOURCES = 

# clean backup files
CLEANFILES = *~ .ver $(VERSIONINC)

AM_CFLAGS = -Wall

version.h:
	echo '/* Auto-generated at build time. */' > $(VERSIONINC)
	echo "#define COMPILE_TIME \"`date '+%B %d, %Y at %H:%M'`\"" >> $(VERSIONINC)
	echo '#define COMPILE_BY "$(USERNAME)"'  >> $(VERSIONINC)
	if (test -n "$(SVNVERSION)" || test -f .svn); then \
		echo -n '#define BUILD "' >> $(VERSIONINC); \
		svnversion -n >> $(VERSIONINC); \
		echo '"' >> $(VERSIONINC); \
	else \
		echo '#define BUILD " "' >> $(VERSIONINC); \
	fi
	if ( test -z "$(host_alias)" ); then \
		echo "#define ARCH \"`uname -m`\"" >> $(VERSIONINC); \
	else \
	  echo "#define ARCH \"$(host_alias)\"" >> $(VERSIONINC); \
	fi

