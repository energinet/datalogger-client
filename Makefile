# (C) LIAB ApS 2009 - 2011

SHELL = /bin/sh
PWD := $(shell pwd)

CROSS_COMPILE = arm-unknown-linux-gnu

SUBAPPS = sqlite expat asocket logdb xmlparser aeeprom contdaem cmddb db-module socketmod en_modules templatemod

BUILDDIR   = buildroot
DEVICEDIR  = deviceroot
DEVICEROOT = $(PWD)/$(DEVICEDIR)
BUILDROOT  = $(PWD)/$(BUILDDIR)
COMPONENTNAME = contdaem
COPY_LIBS =
CONFIGURE_ARM_OPTS = --host=$(CROSS_COMPILE) --with-libraries=$(BUILDROOT)/usr/lib --with-includes=$(BUILDROOT)/usr/include
CONFIGURE_OPTS =  --includedir=/usr/include --sysconfdir=/etc --bindir=/usr/bin  --libdir=/usr/lib --with-plugindir=/usr/share/control-daemon/plugins 
CONFIGURE_EXTRA =
INSTALLSTR = install-strip

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
	$(MAKE) -C  $* DB_FILE="/jffs2/bigdb.sqll" DESTDIR=$(BUILDROOT) $(INSTALLSTR)
	-rm $(BUILDROOT)/usr/lib/*.la

%-clean: 
	$(MAKE) -C $* clean

%-distclean: %-clean
	rm $*/Makefile 
	rm $*/configure
	
sqlite-autoconf-3070603.tar.gz:
	wget http://www.sqlite.org/sqlite-autoconf-3070603.tar.gz
	
sqlite: sqlite-autoconf-3070603.tar.gz
	tar xvf sqlite-autoconf-3070603.tar.gz
	-rm -r sqlite
	mv sqlite-autoconf-3070603 sqlite

sqlite/configure.ac: sqlite

expat-2.0.1.tar.gz:
	wget http://sourceforge.net/projects/expat/files/expat/2.0.1/expat-2.0.1.tar.gz

expat: expat-2.0.1.tar.gz
	tar xvf expat-2.0.1.tar.gz
	-rm -r expat
	mv expat-2.0.1 expat

expat/configure: expat

#$(BUILDROOT) cp-so
deviceroot:
	-rm -r $(DEVICEROOT)
	cp -ra $(BUILDROOT) $(DEVICEROOT)
	-rm -r $(DEVICEROOT)/usr/share/control-daemon/plugins/*.la
	-rm -r $(DEVICEROOT)/usr/share/control-daemon/plugins/*.a
	-rm -r $(DEVICEROOT)/usr/lib/*.la
	-rm -r $(DEVICEROOT)/usr/lib/*.a
	-rm -r $(DEVICEROOT)/usr/include
	-rm -r $(DEVICEROOT)/usr/remove
	-rm -r $(DEVICEROOT)/usr/lib/pkgconfig

#cp-so:
#	mkdir -p $(BUILDROOT)/usr/lib
#	list='$(COPY_LIBS)'; for libs in $$list; do \
#	cp -a armlib/lib/lib$$libs*.so* $(BUILDROOT)/usr/lib ;\
#	done

tar: deviceroot
	cd $(DEVICEROOT) ; tar cvzf ../$(COMPONENTNAME).tar.gz *
	ls -l  $(COMPONENTNAME).tar.gz
	ls -lh  $(COMPONENTNAME).tar.gz

clean:  $(addsuffix -clean, $(SUBAPPS))
	-rm -r $(DEVICEROOT)
	-rm -r $(BUILDROOT)

distclean: $(addsuffix -distclean, $(SUBAPPS))
	-rm -r $(DEVICEROOT)
	-rm -r $(BUILDROOT)
	-rm -r expat sqlite
	-rm contdaem.tar.gz expat-2.0.1.tar.gz sqlite-autoconf-3070603.tar.gz

#Exceptions
expat-mk: INSTALLSTR = installlib

