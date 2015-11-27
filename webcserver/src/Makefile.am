VERSION = 0
PATCHLEVEL = 1
VERSIONINC = version.h
#cgibin_PROGRAMS = control.cgi process.cgi live.cgi grafer.cgi pair.cgi

if SDVP
EXTRACGIS = calib.cgi
else
EXTRACGIS = cmd.cgi  cmd_json.cgi export.cgi dbfile.cgi
endif


cgibin_PROGRAMS = live.cgi grafer.cgi pair.cgi relay.cgi $(EXTRACGIS)


BUILT_SOURCES = version.h


#control_cgi_SOURCES = control.c 
#process_cgi_SOURCES = process.c 
live_cgi_SOURCES = live.c  siteutil.c ethmacget.c
grafer_cgi_SOURCES = grafer.c siteutil.c ethmacget.c
pair_cgi_SOURCES = pair.c  siteutil.c ethmacget.c
relay_cgi_SOURCES = relay.c  siteutil.c ethmacget.c
calib_cgi_SOURCES = calib.c  siteutil.c ethmacget.c
export_cgi_SOURCES = export.c  siteutil.c ethmacget.c
dbfile_cgi_SOURCES = dbfile.c siteutil.c ethmacget.c
cmd_cgi_SOURCES = cmd.c siteutil.c ethmacget.c
cmd_json_cgi_SOURCES = cmd_json.c siteutil.c ethmacget.c json.c


#control_cgi_LDADD = $(DEPS_LIBS) -L../qDecoder-9.0.3/src -lqDecoder


# clean backup files
CLEANFILES = *~ .ver $(VERSIONINC)

AM_CFLAGS = $(DEPS_CFLAGS) -Wall -Werror  -Wstrict-prototypes 

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

