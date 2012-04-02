VERSION = 0
PATCHLEVEL = 1
VERSIONINC = version.h




lib_LTLIBRARIES  = libmodbus.la 
libmodbus_la_SOURCES =  msgpipe.c

plugin_LTLIBRARIES  = modmodbus.la

modmodbus_la_SOURCES = modmodbus.c msgpipe.c
modmodbus_la_LDFLAGS   = -module -avoid-version 
modmodbus_la_LIBADD = -lmodbus
modmodbus_la_DEPENDENCIES = libmodbus.la

bin_PROGRAMS = modbusd modbusclient modbusutil
BUILT_SOURCES = version.h

modbusd_SOURCES = \
	modbusd.c \
	pidfile.c \
	port/portserial.c \
	port/portother.c \
	port/portevent.c \
	port/porttimer.c \
	master/mb-master.c \
        master/rtu/mbrtu.c \
	master/rtu/mbcrc.c \
	master/functions/mbmaster_coils.c \
	master/functions/mbmaster_input.c \
	master/functions/mbmaster_holding.c \
	master/functions/mbmaster_other.c

modbusd_LDADD = -lmodbus
modbusd_DEPENDENCIES = libmodbus.la

modbusclient_SOURCES = modbusclient.c
modbusclient_LDADD = -lmodbus
modbusclient_DEPENDENCIES = libmodbus.la

modbusutil_SOURCES = modbusutil.c
modbusutil_LDADD = -lmodbus 
modbusutil_DEPENDENCIES = libmodbus.la

# clean backup files
CLEANFILES = *~ .ver $(VERSIONINC)
AM_CFLAGS = -Wall -Iport -Imaster/rtu -Imodbus/ascii -Imaster/include -MD

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

