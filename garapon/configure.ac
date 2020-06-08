#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([garapon],[1.0])
AC_CONFIG_SRCDIR([garapon.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

if test -f "$srcdir/BONNOU"; then
	AC_DEFINE([BONNOU],[3],[Define to 3 if you have bonnou])
	garapon_cv_bonnou=yes
else
	garapon_cv_bonnou=no
fi

AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_RANLIB
AC_PROG_GREP
AC_PROG_EGREP

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS([err.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([menu.h], [LIBS="-lmenu -lcurses $LIBS"])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPES([struct timespec])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([srandomdev])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
