# (C) LIAB ApS 2009 - 2012

SHELL = /bin/sh
PWD := $(shell pwd)

CROSS_COMPILE = arm-unknown-linux-gnu
PARALLEL = 3 #Parallel MAKE (CPU cnt+1)

SUBAPPS = asocket logdb xmlparser aeeprom contdaem cmddb wdt db-local db-module webcserver socketmod en_modules templatemod licon licon-mod rpfchart-arm modbusd endata enstyrerapp rpclient

SUBLIBS = gsoap qDecoder sqlite expat

BUILDDIR   = buildroot
DEVICEDIR  = deviceroot
DEVICEROOT = $(PWD)/$(DEVICEDIR)
BUILDROOT  = $(PWD)/$(BUILDDIR)
COMPONENTNAME = contdaem

CONFIGURE_ARM_OPTS = --host=$(CROSS_COMPILE) --with-libraries=$(PWD)/armlib/lib:$(BUILDROOT)/usr/lib --with-includes=$(PWD)/armlib/include:$(BUILDROOT)/usr/include
CONFIGURE_OPTS =  --includedir=/usr/include --sysconfdir=/etc --bindir=/usr/bin  --libdir=/usr/lib --with-plugindir=/usr/share/control-daemon/plugins 
CONFIGURE_EXTRA =
INSTALLSTR = install-strip
LIBBASE = armlib

all: sublibs subapps tar

sublibs: $(addsuffix -lib, $(SUBLIBS))
subapps: $(addsuffix -mk, $(SUBAPPS))

.PRECIOUS: %/configure
%/configure: %/configure.ac
	cd $* ; autoreconf --install  

.PRECIOUS: %/Makefile
%/Makefile: %/configure
	cd $* ; ./configure $(CONFIGURE_ARM_OPTS) $(CONFIGURE_OPTS) $(CONFIGURE_EXTRA)

%-mk: %/Makefile
	mkdir -p $(BUILDROOT)
	$(MAKE) -j$(PARALLEL) -C $* DB_FILE="/jffs2/bigdb.sql" DESTDIR=$(BUILDROOT) $(INSTALLSTR)
	-rm $(BUILDROOT)/usr/lib/*.la

%-lib: %/Makefile
	mkdir -p $(BUILDROOT)
	$(MAKE) -j$(PARALLEL) -C  $* DB_FILE="/jffs2/bigdb.sql" DESTDIR=$(BUILDROOT) $(INSTALLSTR)
	-rm $(BUILDROOT)/usr/lib/*.la

#%-install:
#	mkdir -p $(DEVICEROOT)
#	$(MAKE) -C  $* DESTDIR=$(DEVICEROOT) install

%-clean: 
	$(MAKE) -C $* clean

%-distclean: %-clean
	rm $*/Makefile 
	rm $*/configure

deviceroot:
	-rm -fr $(DEVICEROOT)
	cp -ra $(BUILDROOT) $(DEVICEROOT)
	-rm -fr $(DEVICEROOT)/usr/share/control-daemon/plugins/*.la
	-rm -fr $(DEVICEROOT)/usr/share/control-daemon/plugins/*.a
	-rm -fr $(DEVICEROOT)/usr/lib/*.la
	-rm -fr $(DEVICEROOT)/usr/lib/*.a
	-rm -fr $(DEVICEROOT)/usr/include
	-rm -fr $(DEVICEROOT)/usr/remove
	-rm -fr $(DEVICEROOT)/usr/lib/pkgconfig

tar: deviceroot
	cd $(DEVICEROOT) ; tar cvzf ../$(COMPONENTNAME).tar.gz *
	ls -l  $(COMPONENTNAME).tar.gz
	ls -lh  $(COMPONENTNAME).tar.gz

clean:  $(addsuffix -clean, $(SUBAPPS))
	-rm -fr $(DEVICEROOT)
	-rm -fr $(BUILDROOT)

distclean: $(addsuffix -distclean, $(SUBAPPS))
	-rm -fr $(DEVICEROOT)
	-rm -fr $(BUILDROOT)
	-rm -fr expat sqlite qDecoder gsoap-arm
	-rm $(COMPONENTNAME).tar.gz
	-find -name config.guess -exec rm {} \;
	-find -name config.log -exec rm {} \;
	-find -name config.status -exec rm {} \;
	-find -name config.sub -exec rm {} \;
	-find -name '*~' -exec rm {} \;
	-find -name '#*#' -exec rm {} \;
	-find -name aclocal.m4 -exec rm {} \;
	-find -name autom4te.cache -exec rm -fr {} \; -a -prune
	-find -name .deps -exec rm -fr {} \; -a -prune
	-find -name depcomp -exec rm {} \;
	-find -name install-sh -exec rm {} \;
	-find -name libtool -exec rm {} \;
	-find -name ltmain.sh -exec rm {} \;
	-find -name missing -exec rm {} \;
	
cleansrc:
	-rm -fr  expat-2.0.1.tar.gz sqlite-autoconf-3070603.tar.gz qDecoder-10.1.1.tar.gz gsoap_2.7.16.zip

#qDecoder Special Start
qDecoder-10.1.1.tar.gz:
	wget ftp://ftp.qdecoder.org/pub/qdecoder/qDecoder-10.1.1.tar.gz
qDecoder: qDecoder-10.1.1.tar.gz
	tar xvf qDecoder-10.1.1.tar.gz
	mv qDecoder-10.1.1 qDecoder
	rm qDecoder/configure
	mkdir -p $(BUILDROOT)/usr/include $(BUILDROOT)/usr/lib
qDecoder/configure.ac: qDecoder
qDecoder-lib: INSTALLSTR = install
qDecoder/Makefile: CONFIGURE_OPTS = --disable-socket --disable-ipc --disable-datastructure  --libdir=$(BUILDROOT)/usr/lib --prefix=$(PWD) --includedir=$(BUILDROOT)/usr/include --host=$(CROSS_COMPILE)
#qDecoder Special End

#gSOAP Special Start
gsoap_2.7.16.zip:
	wget http://downloads.sourceforge.net/project/gsoap2/gSOAP/gSOAP%202.7.16%20stable/gsoap_2.7.16.zip
gsoap: gsoap_2.7.16.zip
	unzip gsoap_2.7.16.zip
	mv gsoap-2.7 gsoap
	rm gsoap/configure
	cd gsoap; patch -p1 < ../gsoap-patch 
gsoap/Makefile: CONFIGURE_OPTS = --bindir=/usr/remove --libdir=/usr/lib --includedir=/usr/include --datadir=/usr/remove LDFLAGS=-L$(PWD)/$(LIBBASE)/lib CPPFLAGS="-I$(PWD)/$(LIBBASE)/include"
gsoap/Makefile: CONFIGURE_ARM_OPTS = --host=$(CROSS_COMPILE)
gsoap/configure.ac: gsoap
gsoap-lib: INSTALLSTR = install
gsoap-lib: PARALLEL = 1
#gSOAP Special End

#sqlite Special Start
sqlite-autoconf-3070603.tar.gz:
	wget http://www.sqlite.org/sqlite-autoconf-3070603.tar.gz
sqlite: sqlite-autoconf-3070603.tar.gz
	tar xvf sqlite-autoconf-3070603.tar.gz
	-rm -fr sqlite
	mv sqlite-autoconf-3070603 sqlite
sqlite/configure.ac: sqlite
#sqlite Special End

#expat Special Start
expat-2.0.1.tar.gz:
	wget http://sourceforge.net/projects/expat/files/expat/2.0.1/expat-2.0.1.tar.gz
expat: expat-2.0.1.tar.gz
	tar xvf expat-2.0.1.tar.gz
	-rm -fr expat
	mv expat-2.0.1 expat
expat/configure: expat
expat-lib: INSTALLSTR = installlib
#expat Special End

#gSOAP depencies special Start
rpclient-mk: PARALLEL = 1
rpfchart-arm-mk: PARALLEL = 1
endata-mk: PARALLEL = 1
#gSOAP depencies special End
