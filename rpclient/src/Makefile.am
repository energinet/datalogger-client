VERSIONINC = version.h
BUILT_SOURCES = version.h

bin_PROGRAMS = rpclient rpclientutil

rpclient_SOURCES = md5.c rpclient_soap.c rpfileutil.c rpclient.c rpclient_db.c rpclient_cmd.c soapC.c soapClient.c pidfile.c rpclient_stfile.c $(DESTDIR)@datadir@/gsoap/plugin/md5evp.c $(DESTDIR)@datadir@/gsoap/plugin/httpda.c rpclient_crypto.c
#plugin/httpda.c     



rpclientutil_SOURCES = md5.c rpclient_soap.c rpfileutil.c rpclientutil.c rpclient_db.c rpclient_cmd.c soapC.c soapClient.c pidfile.c rpclient_stfile.c $(DESTDIR)@datadir@/gsoap/plugin/md5evp.c $(DESTDIR)@datadir@/gsoap/plugin/httpda.c rpclient_crypto.c


# boxcmdq.c
rpclient_LDFLAGS = -g -lm -lgsoapssl  -lz -lssl -lcrypto
rpclient_c_DEPENDENCIES = version.h

rpclientutil_LDFLAGS = -g -lm -lgsoapssl  -lz -lssl -lcrypto
#rpclientutil_DEPENDENCIES = version.h
#AM_CFLAGS = -Wall  -I../gsoap  -DWITH_GZIP -DWITH_OPENSSL -O -g -DDEBUG
AM_CFLAGS = -Wall  -I../gsoap  -DWITH_GZIP -DWITH_OPENSSL -O -g -I$(DESTDIR)@datadir@/gsoap

cert_DATA = \
    ../cert/cacert.pem


CLEANFILES = *~ .ver $(VERSIONINC)

.PHONY: version.h

version.h:
	echo '/* Auto-generated at build time. */' > $(VERSIONINC)
	echo "#define COMPILE_TIME \"`date '+%B %d, %Y at %H:%M'`\"" >> $(VERSIONINC)
	echo '#define COMPILE_BY "$(USERNAME)"'  >> $(VERSIONINC)
	echo -n '#define BUILD "' >> $(VERSIONINC); 	
	if (test -f ../../version.txt); then \
	    cat ../../version.txt  | xargs echo -n >> $(VERSIONINC); \
	else \
	    if (test -d ../.git) ; then \
		if (git describe --tags 2> /dev/null) ; then \
		    git describe --tags | xargs echo -n >> $(VERSIONINC); \
	        else \
	            git log -1 --pretty=format:'%h - %s (%ci)' --abbrev-commit| xargs echo -n >> $(VERSIONINC); \
		fi ; \
	    else \
		svnversion | xargs echo -n >> $(VERSIONINC); \
	    fi ; \
        fi ;
	echo '"' >> $(VERSIONINC); 
	echo -n '#define BUILDID "'  >> $(VERSIONINC); 
	if (test -d ../.git) ; then \
		if (git describe --tags 2> /dev/null) ; then \
		    git describe --tags | xargs echo -n >> $(VERSIONINC); \
	        else \
	            git log -1 --pretty=format:'%h - %s (%ci)' --abbrev-commit| xargs echo -n >> $(VERSIONINC); \
		fi ; \
	    else \
		svnversion | xargs echo -n >> $(VERSIONINC); \
	fi ; 
	echo '"' >> $(VERSIONINC); 
	echo '#define AC_VERSION "$(VERSION)"' >> $(VERSIONINC);
	if ( test -z "$(host_alias)" ); then \
		echo "#define ARCH \"`uname -m`\"" >> $(VERSIONINC); \
	else \
	  echo "#define ARCH \"$(host_alias)\"" >> $(VERSIONINC); \
	fi
