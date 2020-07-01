#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([tslib], [1.22+], [tslib@lists.infradead.org], [tslib], [http://tslib.org])
# AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE([dist-bzip2 dist-xz subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES(yes)]) # use silent rules where available - automake 1.11
AC_CONFIG_SRCDIR([src/ts_close.c])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR([m4/internal])

PACKAGE_DESCRIPTION="Touchscreen Access Library"
AC_SUBST(PACKAGE_DESCRIPTION)

# Set our default flags variables
TS_DEFAULT_FLAGS

if test "x${enable_dependency_tracking}" = "x"; then
	enable_dependency_tracking=no
fi

# Check for host os
AC_CANONICAL_HOST
build_linux=no
build_freebsd=no
build_hurd=no
build_haiku=no
build_windows=no
build_unknown=no
case "${host_os}" in
	linux*)
		build_linux=yes
		;;
	freebsd*|k*bsd-gnu*)
		build_freebsd=yes
		;;
	gnu*)
		build_hurd=yes
		;;
	beos*|*haiku*)
		build_haiku=yes
		;;
	mingw*)
		build_windows=yes
		;;
	*)
		build_unknown=yes
		;;
esac
AM_CONDITIONAL([LINUX], [test "$build_linux" = "yes"])
AM_CONDITIONAL([FREEBSD], [test "$build_freebsd" = "yes"])
AM_CONDITIONAL([HURD], [test "$build_hurd" = "yes"])
AM_CONDITIONAL([HAIKU], [test "$build_haiku" = "yes"])
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])

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
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/ioctl.h sys/time.h unistd.h stdint.h sys/types.h errno.h dirent.h])
AC_CHECK_HEADERS([linux/spi/cy8mrln.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T
AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MMAP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gettimeofday memmove memset munmap select strcasecmp strchr strdup strtoul strtol strsep])
AM_CONDITIONAL(HAVE_STRSEP, test x$HAVE_STRSEP = xyes)

# filters
#########
TSLIB_CHECK_MODULE([debounce], [yes], [Enable building of debounce filter])
TSLIB_CHECK_MODULE([dejitter], [yes], [Enable building of dejitter filter])
TSLIB_CHECK_MODULE([evthres], [yes], [Enable building of evthres filter])
TSLIB_CHECK_MODULE([iir], [yes], [Enable building of IIR filter])
TSLIB_CHECK_MODULE([linear], [yes], [Enable building of linear scaling])
TSLIB_CHECK_MODULE([median], [yes], [Enable building of median filter])
TSLIB_CHECK_MODULE([pthres], [yes], [Enable building of pthres filter])
TSLIB_CHECK_MODULE([skip], [yes], [Enable building of skip filter])
TSLIB_CHECK_MODULE([lowpass], [yes], [Enable building of lowpass filter])
TSLIB_CHECK_MODULE([invert], [yes], [Enable building of invert filter])
TSLIB_CHECK_MODULE([variance], [yes], [Enable building of variance filter])

# hardware access modules
#########################
# generic, recommended
TSLIB_CHECK_MODULE([input], [yes], [Enable building of generic input raw module (Linux /dev/input/eventN support)])

# userspace device drivers, enabled by default (may become disabled by default in the future)
TSLIB_CHECK_MODULE([touchkit], [yes], [Enable building of serial TouchKit raw module (Linux /dev/ttySX support)])
TSLIB_CHECK_MODULE([waveshare], [yes], [Enable building of WaveShare raw module (Linux /dev/hidrawN support)])

# userspace device drivers, disabled by default
TSLIB_CHECK_MODULE([input-evdev], [no], [Enable building of generic evdev input module (Linux /dev/input/eventN support)])
TSLIB_CHECK_MODULE([tatung], [no], [Enable building of tatung raw module (Tatung Webpad support)])
TSLIB_CHECK_MODULE([ucb1x00], [no], [Enable building of ucb1x00 raw module (UCB1x00 support)])
TSLIB_CHECK_MODULE([mk712], [no], [Enable building of mk712 raw module (Hi tachi support)])
TSLIB_CHECK_MODULE([h3600], [no], [Enable building of h3600 raw module (HP iPaq H3600 support)])
TSLIB_CHECK_MODULE([dmc], [no], [Enable building of dmc raw module (HP iPaq DMC support)])
TSLIB_CHECK_MODULE([linear-h2200], [no], [Enable building of linearizing filter for iPAQ h2200])
TSLIB_CHECK_MODULE([corgi], [no], [Enable building of corgi raw module (Sharp Zaurus sl-c7x0 support)])
TSLIB_CHECK_MODULE([collie], [no], [Enable building of collie raw module (Sharp Zaurus sl-5500/5000d support)])
TSLIB_CHECK_MODULE([arctic2], [no], [Enable building of arctic2 raw module (IBM Arctic II support)])
TSLIB_CHECK_MODULE([dmc_dus3000], [no], [Enable building of dmc_dus3000 raw module (DMC DUS Series support)])
TSLIB_CHECK_MODULE([cy8mrln-palmpre], [no], [Enable building of cy8mrln-palmpre raw module])
TSLIB_CHECK_MODULE([galax], [no], [Enable building of HID USB eGalax raw module (Linux /dev/hiddevN support)])
TSLIB_CHECK_MODULE([one-wire-ts-input], [no], [Enable building of FriendlyARM one-wire raw module])

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

AC_MSG_CHECKING([whether to enable tools])
AC_ARG_ENABLE([tools], AS_HELP_STRING([--disable-tools], [Disable tools (ts_uinput)]), [], [enable_tools=yes])
AM_CONDITIONAL([ENABLE_TOOLS], [test "$enable_tools" = "yes"])
AC_MSG_RESULT($enable_tools)

AC_DEFUN([AX_DEFINE_SUBST], [
AC_DEFINE_UNQUOTED([$1], [$2], [$3])
AC_SUBST([$1], ['$2'])
])

AC_ARG_WITH(sdl2,
  [AS_HELP_STRING([--with-sdl2], [build graphical tools like ts_calibrate using SDL2])],
  [with_sdl2=$withval],
  [with_sdl2=no])

AM_CONDITIONAL(SDL, test x"$with_sdl2" = "xyes")
AS_IF([test "x$with_sdl2" = "xyes"], [
  AC_DEFINE(HAVE_SDL2,,[Use SDL2.])
  AC_CHECK_LIB([SDL2], [SDL_Init], [], [exit 1])
])

# libts Library versioning
# increment if the interface changed
LT_CURRENT=10
# increment if anything changed. set to 0 if current was incremented!
LT_REVISION=4
# increment if the interface change is backwards compatible (an addition). set to 0 if not.
LT_AGE=10
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AX_DEFINE_SUBST([LIBTS_VERSION_CURRENT], [$LT_CURRENT], [library current number])
AX_DEFINE_SUBST([LIBTS_VERSION_REVISION], [$LT_REVISION], [library revision number])
AX_DEFINE_SUBST([LIBTS_VERSION_AGE], [$LT_AGE], [library age number])

AC_CONFIG_FILES([Makefile
                 etc/Makefile
                 plugins/Makefile
                 src/Makefile
                 tests/Makefile
                 tools/Makefile
                 doc/Makefile
                 tslib.pc:tslib.pc.in])
AC_OUTPUT
