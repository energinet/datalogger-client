VERSIONINC = version.h
BUILT_SOURCES = version.h

bin_PROGRAMS = rpclient rpclientutil

rpclient_SOURCES = md5.c rpclient_soap.c rpfileutil.c rpclient.c rpclient_db.c rpclient_cmd.c soapC.c soapClient.c pidfile.c rpclient_stfile.c 
rpclientutil_SOURCES = md5.c rpclient_soap.c rpfileutil.c rpclientutil.c rpclient_db.c rpclient_cmd.c soapC.c soapClient.c pidfile.c rpclient_stfile.c 


# boxcmdq.c
rpclient_LDFLAGS = -g -lm -lgsoapssl  -lz -lssl -lcrypto
rpclientutil_LDFLAGS = -g -lm -lgsoapssl  -lz -lssl -lcrypto
#AM_CFLAGS = -Wall  -I../gsoap  -DWITH_GZIP -DWITH_OPENSSL -O -g -DDEBUG
AM_CFLAGS = -Wall  -I../gsoap  -DWITH_GZIP -DWITH_OPENSSL -O -g

cert_DATA = \
    ../cert/cacert.pem


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
