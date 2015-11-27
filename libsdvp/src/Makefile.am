SSL_LIBS = -lssl -lcrypto -lz -lresolv 
STROPHE_LIBS =  $(PARSER_LIBS) $(SSL_LIBS)

lib_LTLIBRARIES  = libsdvp.la 
libsdvp_la_SOURCES = 	sdvp.c sdvp_config.c sdvp_map.c sdvp_item.c sdvp_helper.c sdvptypes.c sdvp_register.c sdvp_rpchelper.c
libsdvp_la_LIBADD = $(STROPHE_LIBS) 

include_HEADERS = sdvp.h sdvptypes.h sdvp_register.h sdvp_map.h sdvp_item.h 

VERSIONINC = version.h
BUILT_SOURCES = version.h


# clean backup files
CLEANFILES = *~ .ver $(VERSIONINC)

#AM_CFLAGS = -Wall -Werror -lstrophe -std=c99

AM_CFLAGS = -Wall  -lstrophe -std=c99

.PHONY: version.h
version.h:
	echo '/* Auto-generated at build time. */' > $(VERSIONINC)
	echo "#define COMPILE_TIME \"`date '+%B %d, %Y at %H:%M'`\"" >> $(VERSIONINC)
	echo '#define COMPILE_BY "$(USERNAME)"'  >> $(VERSIONINC)
	echo -n '#define COPYRIGHTYEAR "2008-' >> $(VERSIONINC); \
	date +%G\" >> $(VERSIONINC); \
	echo -n '#define BUILD "' >> $(VERSIONINC); 
	if git status ; then \
	    if (git describe --tags 2> /dev/null) ; then \
	        git describe --tags | sed 's/\-/l/2'| sed  -e 's/\l.*//' | xargs echo -n >> $(VERSIONINC); \
	    else \
	        git log -1 --pretty=format:'%h - %s (%ci)' --abbrev-commit  | xargs echo -n >> $(VERSIONINC); \
	    fi ; \
	else  \
	    if (test -n "$(SVNVERSION)" || test -f .svn); then \
		svnversion ../.. -n >> $(VERSIONINC); \
	    else \
		echo -n '???' >> $(VERSIONINC); \
	    fi;\
	fi;	
	echo '"' >> $(VERSIONINC); \
	if ( test -z "$(host_alias)" ); then \
		echo "#define ARCH \"`uname -m`\"" >> $(VERSIONINC); \
	else \
	  echo "#define ARCH \"$(host_alias)\"" >> $(VERSIONINC); \
	fi 


