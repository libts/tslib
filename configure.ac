#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(tslib, 1.0.0, kergoth@handhelds.org)
# AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(dist-bzip2)
AC_CONFIG_SRCDIR([src/ts_close.c])
AC_CONFIG_HEADERS(config.h)

PACKAGE_DESCRIPTION="Touchscreen Access Library"
AC_SUBST(PACKAGE_DESCRIPTION)

# Set our default flags variables
TS_DEFAULT_FLAGS

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
TS_CC_HIDDEN_VISIBILITY
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_DISABLE_STATIC
AC_ENABLE_SHARED
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/ioctl.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MMAP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gettimeofday memmove memset munmap select strcasecmp strchr strdup strtoul])

# filters
TSLIB_CHECK_MODULE([linear], [yes], [Enable building of linear scaling])
TSLIB_CHECK_MODULE([dejitter], [yes], [Enable building of dejitter filter])
TSLIB_CHECK_MODULE([linear-h2200], [yes], [Enable building of linearizing filter for iPAQ h2200])
TSLIB_CHECK_MODULE([variance], [yes], [Enable building of variance filter])
TSLIB_CHECK_MODULE([pthres], [yes], [Enable building of pthres filter])

# hardware access modules
TSLIB_CHECK_MODULE([ucb1x00], [yes], [Enable building of ucb1x00 raw module (UCB1x00 support)])
TSLIB_CHECK_MODULE([cy8mrln-palmpre], [no], [Enable building of cy8mrln-palmpre raw module])
TSLIB_CHECK_MODULE([corgi], [yes], [Enable building of corgi raw module (Sharp Zaurus sl-c7x0 support)])
TSLIB_CHECK_MODULE([collie], [yes], [Enable building of collie raw module (Sharp Zaurus sl-5500/5000d support)])
TSLIB_CHECK_MODULE([h3600], [yes], [Enable building of h3600 raw module (HP iPaq H3600 support)])
TSLIB_CHECK_MODULE([mk712], [yes], [Enable building of mk712 raw module (Hi tachi support)])
TSLIB_CHECK_MODULE([arctic2], [yes], [Enable building of arctic2 raw module (IBM Arctic II support)])
TSLIB_CHECK_MODULE([tatung], [yes], [Enable building of tatung raw module (Tatung Webpad support)])
TSLIB_CHECK_MODULE([dmc], [yes], [Enable building of dmc raw module (HP iPaq DMC support)])
TSLIB_CHECK_MODULE([input], [yes], [Enable building of generic input raw module (Linux /dev/input/eventN support)])
TSLIB_CHECK_MODULE([touchkit], [yes], [Enable building of serial TouchKit raw module (Linux /dev/ttySX support)])

AC_MSG_CHECKING([where to place modules])
AC_ARG_WITH(plugindir,
        AS_HELP_STRING([--with-plugindir=ARG],
                [Where to look for plugins (default=${libdir}/ts)]),
        [PLUGIN_DIR=$withval],
        [PLUGIN_DIR='${libdir}/ts'])
AC_MSG_RESULT($PLUGIN_DIR)
AC_SUBST(PLUGIN_DIR)

AC_MSG_CHECKING([whether to enable debugging])
AC_ARG_ENABLE(debug,
	AS_HELP_STRING(--enable-debug
		Enable debug messages from filters (default=no)), 
	,
	[enable_debug="no"])
AC_MSG_RESULT($enable_debug)
if test "$enable_debug" = "yes"; then
	DEBUGFLAGS="-g -ggdb -DDEBUG"
else
	DEBUGFLAGS=""
fi
AC_SUBST(DEBUGFLAGS)

LIBFLAGS="-DTSLIB_INTERNAL"
AC_SUBST(LIBFLAGS)

TS_CONF='${sysconfdir}/ts.conf'
AC_SUBST(TS_CONF)

TS_POINTERCAL='${sysconfdir}/pointercal'
AC_SUBST(TS_POINTERCAL)

# Library versioning
LT_RELEASE=1.0
LT_CURRENT=0
LT_REVISION=0
LT_AGE=0
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

AC_CONFIG_FILES([Makefile
                 etc/Makefile
                 plugins/Makefile
                 src/Makefile
                 tests/Makefile
                 tslib.pc:tslib.pc.in
                 tslib-$LT_RELEASE.pc:tslib.pc.in])
AC_OUTPUT
