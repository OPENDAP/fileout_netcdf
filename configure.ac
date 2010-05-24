# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(fileout_netcdf, 1.0.1, opendap-tech@opendap.org)
AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([conf])

AM_INIT_AUTOMAKE(1.10)
AC_CONFIG_TESTDIR(unit-tests, [/usr/local/netcdf/bin])

AC_CONFIG_SRCDIR([FONcTransmitter.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_CHECK_FUNCS([strchr])

# Support for large files?
AC_SYS_LARGEFILE

dnl Checks for specific libraries
AC_CHECK_LIBDAP([3.10.2],
 [
  LIBS="$LIBS $DAP_SERVER_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])
  
AX_PATH_GENERIC([nc], [4.0.0], [],
    [
	LIBS="$LIBS $NC_LIBS"
	LDFLAGS="$LDFLAGS $NC_LDFLAGS"
	CPPFLAGS="$CPPFLAGS $NC_CFLAGS"
	netcdf4_found=1
    ],
    [
	AC_MSG_WARN([Could not find netcdf 4; looking for the older library])
	netcdf4_found=0
    ]
)

if test $netcdf4_found -ne 1
then
    AC_CHECK_NETCDF(
       [
	   LIBS="$LIBS $NC_LIBS"
	   LDFLAGS="$LDFLAGS $NC_LDFLAGS"
	   CPPFLAGS="$CPPFLAGS $NC_CPPFLAGS"
       ],
       [
	   echo "*** Use --with-netcdf for the root netcdf directory."
	   echo "*** Otherwise use --with-netcdf-include switch for includes directory"
	   echo "*** and --with-netcdf-libdir switch for libraries directory."
	   AC_MSG_ERROR([netcdf library and netcdf headers are required.])
       ],[3]
    )
fi

AC_CHECK_BES([3.8.3],
[
],
[ AC_MSG_ERROR([Cannot find bes])
])

AC_CONFIG_FILES([Makefile unit-tests/Makefile unit-tests/test_config.h unit-tests/atlocal])

AC_OUTPUT

