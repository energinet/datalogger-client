AC_PREREQ([2.69])
AC_INIT([restoremodem], [1.0], [info@liab.dk])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/at91_pio.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MMAP
AC_CHECK_FUNCS([strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
