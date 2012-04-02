VERSIONINC = version.h
BUILT_SOURCES = version.h

bin_PROGRAMS = endatautil

endatautil_SOURCES = endatautil.c endata.c soapC.c soapClientLib.c
#endatautil_LDADD = -lendata
endatautil_DEPENDENCIES = libendata.a
lib_LIBRARIES  = libendata.a
libendata_a_SOURCES = endata.c soapC.c soapClientLib.c

include_HEADERS = endata.h


# boxcmdq.c
endatautil_LDFLAGS = -g -lm -lgsoapssl  
AM_CFLAGS = -Wall  -I../gsoap -lssl -DWITH_OPENSSL -O -g 
# -DWITH_GZIP 
CLEANFILES = *~ .ver $(VERSIONINC)

version.h:
	echo '/* Auto-generated at build time. */' > $(VERSIONINC)
	echo "#define COMPILE_TIME \"`date '+%Y-%b-%d %H:%M'`\"" >> $(VERSIONINC)
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
