
bin_PROGRAMS = sdvpd
sdvpd_SOURCES = sdvpd.c sdvpd_handler.c contdaem_client.c pidfile.c sdvpd_xml.c sdvpd_item.c oom_adjust.c
sdvpd_LDADD=../../libsdvp/src/.libs/libsdvp.a ../../libstrophe/.libs/libstrophe.a -lresolv

VERSIONINC = version.h
BUILT_SOURCES = version.h



# clean backup files
CLEANFILES = *~ .ver $(VERSIONINC)

AM_CFLAGS = -Wall -Werror -D_GNU_SOURCE -g 
#AM_CFLAGS = -Wall -Werror -D_GNU_SOURCE -g -DMEMWATCH


.PHONY: version.h

version.h:
	echo '/* Auto-generated at build time. */' > $(VERSIONINC)
	echo "#define COMPILE_TIME \"`date '+%B %d, %Y at %H:%M'`\"" >> $(VERSIONINC)
	echo '#define COMPILE_BY "$(USERNAME)"'  >> $(VERSIONINC)
	if (test -f ../version.txt); then \
		echo -n '#define BUILD "' >> $(VERSIONINC); \
		cat ../version.txt | sed -r 's/^[^0-9]*([0-9a-zA-Z]+).*/\1/' | xargs echo -n >> $(VERSIONINC); \
		echo '"' >> $(VERSIONINC); \
	else \
		echo '#define BUILD " "' >> $(VERSIONINC); \
	fi
	echo -n '#define BUILDTAG "'  >> $(VERSIONINC); 
	git describe --tags	| xargs echo -n >> $(VERSIONINC); 
	echo '"' >> $(VERSIONINC); 
	echo '#define AC_VERSION "$(VERSION)"' >> $(VERSIONINC);
	if ( test -z "$(host_alias)" ); then \
		echo "#define ARCH \"`uname -m`\"" >> $(VERSIONINC); \
	else \
	  echo "#define ARCH \"$(host_alias)\"" >> $(VERSIONINC); \
	fi

