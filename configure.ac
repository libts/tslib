#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(tslib, 0.0.2, kergoth@handhelds.org)
# AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(dist-bzip2)
AC_CONFIG_SRCDIR([src/ts_close.c])
AM_CONFIG_HEADER(config.h)

PACKAGE_DESCRIPTION="Touchscreen Access Library"
AC_SUBST(PACKAGE_DESCRIPTION)

# Set our default flags variables
TS_DEFAULT_FLAGS

# Checks for programs.
AC_PROG_CXX
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
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero gettimeofday memmove memset munmap select strcasecmp strchr strdup strtoul])

# filters
AC_MSG_CHECKING([whether linear modules is requested])
AC_ARG_ENABLE(linear, 
	AS_HELP_STRING([--enable-linear],
		[Enable building of linear scaling (default=yes)]),
	[linear_module=$enableval],
	[linear_module=yes])
AC_MSG_RESULT($linear_module)
AM_CONDITIONAL(ENABLE_LINEAR_MODULE, test "$linear_module" = "yes")

AC_MSG_CHECKING([whether dejitter module is requested]) 
AC_ARG_ENABLE(dejitter,
	AS_HELP_STRING([--enable-dejitter],
		[Enable building of dejitter filter (default=yes)]),
	[dejitter_module=$enableval],
	[dejitter_module=yes])
AC_MSG_RESULT($dejitter_module)
AM_CONDITIONAL(ENABLE_DEJITTER_MODULE, test "$dejitter_module" = "yes")

AC_MSG_CHECKING([whether linear-h2200 module is requested]) 
AC_ARG_ENABLE(dejitter,
	AS_HELP_STRING([--enable-linear-h2200],
		[Enable building of linearizing filter for iPAQ h2200 (default=yes)]),
	[h2200_linear_module=$enableval],
	[h2200_linear_module=yes])
AC_MSG_RESULT($h2200_linear_module)
AM_CONDITIONAL(ENABLE_H2200_LINEAR_MODULE, test "$h2200_linear_module" = "yes")

AC_MSG_CHECKING([whether variance module is requested])
AC_ARG_ENABLE(variance,
	AS_HELP_STRING([--enable-variance],
		[Enable building of variance filter (default=yes)]),
	[variance_module=$enableval],
	[variance_module=yes])
AC_MSG_RESULT($variance_module)
AM_CONDITIONAL(ENABLE_VARIANCE_MODULE, test "$variance_module" = "yes")

AC_MSG_CHECKING([whether pthres module is requested])
AC_ARG_ENABLE(pthres,
	AS_HELP_STRING([--enable-pthres],
		[Enable building of pthres filter (default=yes)]),
	[pthres_module=$enableval],
	[pthres_module=yes])
AC_MSG_RESULT($pthres_module)
AM_CONDITIONAL(ENABLE_PTHRES_MODULE, test "$pthres_module" = "yes")


# hardware access modules
AC_MSG_CHECKING([whether ucb1x00 module is requested])
AC_ARG_ENABLE(ucb1x00,
	AS_HELP_STRING([--enable-ucb1x00],
		[Enable building of ucb1x00 raw module (UCB1x00 support) (default=yes)]),
	[ucb1x00_module=$enableval],
	[ucb1x00_module=yes])
AC_MSG_RESULT($ucb1x00_module)
AM_CONDITIONAL(ENABLE_UCB1X00_MODULE, test "$ucb1x00_module" = "yes")

AC_MSG_CHECKING([whether corgi module is requested])
AC_ARG_ENABLE(corgi,
        AS_HELP_STRING([--enable-corgi],
                [Enable building of corgi raw module (Sharp Zaurus sl-c7x0 support) (default=yes)]),
        [corgi_module=$enableval],
        [corgi_module=yes])
AC_MSG_RESULT($corgi_module)
AM_CONDITIONAL(ENABLE_CORGI_MODULE, test "$corgi_module" = "yes")

AC_MSG_CHECKING([whether collie module is requested])
AC_ARG_ENABLE(collie,
        AS_HELP_STRING([--enable-collie],
                [Enable building of collie raw module (Sharp Zaurus sl-5500/5000d support) (default=yes)]),
        [collie_module=$enableval],
        [collie_module=yes])
AC_MSG_RESULT($collie_module)
AM_CONDITIONAL(ENABLE_COLLIE_MODULE, test "$collie_module" = "yes")


AC_MSG_CHECKING([whether h3600 module is requested])
AC_ARG_ENABLE(h3600,
        AS_HELP_STRING([--enable-h3600],
                [Enable building of h3600 raw module (HP iPaq H3600 support) (default=yes)]),
        [h3600_module=$enableval],
        [h3600_module=yes])
AC_MSG_RESULT($h3600_module)
AM_CONDITIONAL(ENABLE_H3600_MODULE, test "$h3600_module" = "yes")

AC_MSG_CHECKING([whether mk712 module is requested])
AC_ARG_ENABLE(mk712,
        AS_HELP_STRING([--enable-mk712],
                [Enable building of mk712 raw module (Hi tachi support) (default=yes)]),
        [mk712_module=$enableval],
        [mk712_module=yes])
AC_MSG_RESULT($mk712_module)
AM_CONDITIONAL(ENABLE_MK712_MODULE, test "$mk712_module" = "yes")

AC_MSG_CHECKING([whether arctic2 module is requested])
AC_ARG_ENABLE(arctic2,
        AS_HELP_STRING([--enable-arctic2],
                [Enable building of arctic2 raw module (IBM Arctic II support) (default=yes)]),
        [arctic2_module=$enableval],
        [arctic2_module=yes])
AC_MSG_RESULT($arctic2_module)
AM_CONDITIONAL(ENABLE_ARCTIC2_MODULE, test "$arctic2_module" = "yes")

AC_MSG_CHECKING([whether input module is requested])
AC_ARG_ENABLE(input,
        AS_HELP_STRING([--enable-input],
                [Enable building of generic input raw module (Linux /dev/input/eventN support) (default=yes)]),
        [input_module=$enableval],
        [input_module=yes])
AC_MSG_RESULT($input_module)
AM_CONDITIONAL(ENABLE_INPUT_MODULE, test "$input_module" = "yes")

AC_MSG_CHECKING([where to place modules])
AC_ARG_WITH(plugindir,
        AS_HELP_STRING([--with-plugindir=ARG],
                [Where to look for plugins (default=${libdir}/ts)]),
        [PLUGIN_DIR=$withval],
        [PLUGIN_DIR='${libdir}/ts/'])
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

# Library versioning
LT_RELEASE=0.0
LT_CURRENT=1
LT_REVISION=1
LT_AGE=1
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

AC_CONFIG_FILES([Makefile
                 etc/Makefile
                 plugins/Makefile
                 src/Makefile
                 tests/Makefile
                 tslib-$LT_RELEASE.pc:tslib.pc.in])
AC_OUTPUT
