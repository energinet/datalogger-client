VERSION = 0
PATCHLEVEL = 1
VERSIONINC = version.h
#cgibin_PROGRAMS = control.cgi process.cgi live.cgi grafer.cgi pair.cgi
cgibin_PROGRAMS = live.cgi grafer.cgi pair.cgi relay.cgi calib.cgi



BUILT_SOURCES = version.h


#control_cgi_SOURCES = control.c 
#process_cgi_SOURCES = process.c 
live_cgi_SOURCES = live.c  siteutil.c
grafer_cgi_SOURCES = grafer.c siteutil.c
pair_cgi_SOURCES = pair.c  siteutil.c
relay_cgi_SOURCES = relay.c  siteutil.c
calib_cgi_SOURCES = calib.c  siteutil.c

#control_cgi_LDADD = $(DEPS_LIBS) -L../qDecoder-9.0.3/src -lqDecoder


# clean backup files
CLEANFILES = *~ .ver $(VERSIONINC)

AM_CFLAGS = $(DEPS_CFLAGS) -Wall

version.h:
	echo '/* Auto-generated at build time. */' > $(VERSIONINC)
	echo "#define COMPILE_TIME \"`date '+%B %d, %Y at %H:%M'`\"" >> $(VERSIONINC)
	echo '#define COMPILE_BY "$(USERNAME)"'  >> $(VERSIONINC)
	echo '#define RELEASE "$(VERSION).$(PATCHLEVEL)"' >> $(VERSIONINC)
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

