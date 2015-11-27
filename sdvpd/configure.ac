AC_INIT([sdvpd], [0.2],
        [LIAB ApS <info@liab.dk>],
        [sdvpd])
AC_CONFIG_SRCDIR(src/sdvpd.c)
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL

AC_PROG_LIBTOOL

AC_PATH_PROGS(BASH, bash)

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

dnl Modules...'nuff said. 
AC_ARG_WITH(includes,
  [AC_HELP_STRING(
    [--with-includes=LIST],
    [add additional include paths to eesd. LIST is a colon-separated list of include paths to add e.g. --with-includes=/some/mysql/include:/my/include])
  ],
  [ ac_addl_includes=`echo "$withval" | sed -e 's/:/ /g'` ;
    for ainclude in $ac_addl_includes; do
      if test x"$ac_build_addl_includes" = x ; then
        ac_build_addl_includes="-I$ainclude"
      else
        ac_build_addl_includes="-I$ainclude $ac_build_addl_includes"
      fi
    done
    CPPFLAGS="$CPPFLAGS $ac_build_addl_includes"
  ])

AC_ARG_WITH(libraries,
  [AC_HELP_STRING(
    [--with-libraries=LIST],
    [add additional library paths eesd LIST is a colon-separated list of include paths to add e.g. --with-libraries=/some/mysql/libdir:/my/libs])
  ],
  [ ac_addl_libdirs=`echo "$withval" | sed -e 's/:/ /g'` ;
    for alibdir in $ac_addl_libdirs; do
      if test x"$ac_build_addl_libdirs" = x ; then
        ac_build_addl_libdirs="-L$alibdir"
      else
        ac_build_addl_libdirs="-L$alibdir $ac_build_addl_libdirs"
      fi
    done 
    LDFLAGS="$LDFLAGS $ac_build_addl_libdirs"
  ])




AC_CHECK_HEADER(expat.h,, AC_MSG_ERROR(expat.h not found try using --with-includes ))
AC_CHECK_LIB(expat,XML_SetElementHandler,,AC_MSG_ERROR(expat not found try using --with-libraries))


AC_CHECK_HEADER(zlib.h,, AC_MSG_ERROR(zlib.h not found try using --with-includes ))
AC_CHECK_LIB(z, gzread,,AC_MSG_ERROR(libz not found try using --with-libraries))

AC_CHECK_HEADER(xml_parser.h,, AC_MSG_ERROR(openssl/md5.h not found try using --with-includes ))
AC_CHECK_LIB(crypto, MD5,,AC_MSG_ERROR(libcrypto not found try using --with-libraries))

AC_CHECK_HEADER(openssl/ssl.h, [], [AC_MSG_ERROR([couldn't find openssl headers, openssl required])])
PKG_CHECK_MODULES([check], [check >= 0.9.4], [], [AC_MSG_WARN([libcheck not found; unit tests will not be compilable])])
AC_CHECK_LIB(ssl,SSL_CTX_new,,AC_MSG_ERROR(ssl not found try using --with-libraries))



AC_CHECK_HEADER(strophe.h,, AC_MSG_ERROR(strophe.h not found try using --with-includes ))
AC_CHECK_HEADER(sdvp.h,, AC_MSG_ERROR(sdvp.h not found try using --with-includes ))
dnl AC_CHECK_LIB(sdvp,sdvp_InitiateReplyParameters,,AC_MSG_ERROR(sdvp not found try using --with-libraries))


AC_CHECK_HEADER(asocket.h,, AC_MSG_ERROR(asocket.h not found try using --with-includes ))
AC_CHECK_LIB(asocket, asckt_srv_start,,AC_MSG_ERROR(asocket not found try using --with-libraries))




AC_CHECK_HEADER(xml_parser.h,, AC_MSG_ERROR(xml_parser.h not found try using --with-includes ))
AC_CHECK_HEADER(xml_serialize.h,, AC_MSG_ERROR(xml_serialize.h not found try using --with-includes ))
AC_CHECK_LIB(xmlpar, parse_xml_file,,AC_MSG_ERROR(xmlpar not found try using --with-libraries))
AC_CHECK_LIB(xmlpar, xml_item_create,,AC_MSG_ERROR(xmlpar (serialize) not found try using --with-libraries))

AC_CHECK_HEADER(xml_cntn.h,, AC_MSG_ERROR(xml_cntn.h not found try using --with-includes ))
AC_CHECK_LIB(xmlcntn, spxml_cntn_tx_add_err,,AC_MSG_ERROR(xmlcntn not found try using --with-libraries))

AC_CHECK_HEADER(sqlite3.h,, AC_MSG_ERROR(sqlite3.h not found try using --with-includes ))
AC_CHECK_LIB(sqlite3, sqlite3_open,,AC_MSG_ERROR(sqlite3 not found try using --with-libraries))


AC_CHECK_HEADER(cmddb.h,, AC_MSG_ERROR(cmddb.h not found try using --with-includes ))
AC_CHECK_LIB(cmddb, cmddb_db_open,,AC_MSG_ERROR(cmddb not found try using --with-libraries))


dnl Check if svnversion is avaliable to get the rev number
AC_PATH_PROG(SVNVERSION, svnversion)

plugindir=/usr/share/control-daemon/plugins
AC_MSG_CHECKING(plugindir)
AC_ARG_WITH(plugindir,
  [AC_HELP_STRING(
    [--with-plugindir=DIR],
    [Where to put the plugin modules (default: /usr/share/control-daemon/plugins)])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      plugindir="$withval"
      ;;
    esac])
AC_SUBST(plugindir)
AC_DEFINE_UNQUOTED(PLUGINDIR,"${plugindir}",[Directory where module plugin files reside])
AC_MSG_RESULT(${plugindir})


doxdir=`pwd`/../doxdoc
AC_MSG_CHECKING(doxdir)
AC_ARG_WITH(doxdir,
  [AC_HELP_STRING(
    [--with-doxdir=DIR],
    [Where to put doxygen outout (default: ../doxdir)])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      htmldir="$withval"
      ;;
    esac])
AC_SUBST(doxdir)
AC_DEFINE_UNQUOTED(DOXDIR,"${doxdir}",[Directory where doxygen output reside])
AC_MSG_RESULT(${doxdir})

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
