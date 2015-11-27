AC_PREREQ([2.68])
AC_INIT(libtimeout, 0:1:0, info@liab.dk)
AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_SRCDIR([src/system_timeout.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(rt, timer_create,,AC_MSG_ERROR(librt not found))

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
