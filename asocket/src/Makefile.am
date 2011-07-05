VERSIONINC = version.h

lib_LTLIBRARIES  = libasocket.la
bin_PROGRAMS = asockettest
BUILT_SOURCES = version.h

asockettest_SOURCES = asockettest.c
asockettest_LDADD =  -lasocket

# Main application
libasocket_la_SOURCES = asocket.c asocket_server.c asocket_client.c asocket_trx.c
include_HEADERS = asocket.h asocket_client.h asocket_server.h asocket_trx.h

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

