dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([update-menus/update-menus.cc])

AM_INIT_AUTOMAKE(menu,2.1.27)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE dnl Be nice with packagers, i.e. ourself 
dnl Checks for programs.
AC_PROG_CXX

AC_PROG_INSTALL

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/file.h syslog.h unistd.h libintl.h)

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(mkdir strdup strerror)

AC_CONFIG_FILES([po/Makefile scripts/Makefile po-sections/Makefile doc/Makefile update-menus/Makefile install-menu/Makefile Makefile])
AC_OUTPUT

