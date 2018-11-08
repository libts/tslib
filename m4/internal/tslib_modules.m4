# Copyright (C) 2018 Martin Kepplinger <martink@posteo.de>
# Copyright (C) 2005 Christopher Larson

dnl use: TSLIB_CHECK_MODULE(module, default, description)
AC_DEFUN([TSLIB_CHECK_MODULE],
[
m4_pushdef([UP], m4_translit([$1], [-a-z], [_A-Z]))dnl

AC_MSG_CHECKING([whether $1 module is requested]) 

AC_ARG_ENABLE([$1],
   AC_HELP_STRING([--enable-$1], [$3 (default=$2)]),
   [enable_module=$enableval],
   [enable_module=$2])
AC_MSG_RESULT($enable_module)

AM_CONDITIONAL(ENABLE_[]UP[]_MODULE, test "x$enable_module" = "xyes")
AM_CONDITIONAL(ENABLE_STATIC_[]UP[]_MODULE, test "x$enable_module" = "xstatic")
if test "x$enable_module" = "xstatic" ; then
	AC_DEFINE(TSLIB_STATIC_[]UP[]_MODULE, 1, whether $1 should be build as part of libts or as a 
		separate shared library which is dlopen()-ed at runtime)
fi

if test "$1" = "input-evdev" ; then
	if test "x$enable_module" = "xyes" ; then
		PKG_CHECK_MODULES(LIBEVDEV, [libevdev >= 0.4], [], [AC_MSG_ERROR([input-evdev needs libevdev])])
	elif test "x$enable_module" = "xstatic" ; then
		PKG_CHECK_MODULES(LIBEVDEV, [libevdev >= 0.4], [], [AC_MSG_ERROR([input-evdev needs libevdev])])
	fi
fi

m4_popdef([UP])
])
