#
#  tslib/tests/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

AM_CFLAGS               = -DTS_POINTERCAL=\"@TS_POINTERCAL@\" $(DEBUGFLAGS)
AM_CPPFLAGS		= -I$(top_srcdir)/src

bin_PROGRAMS		= ts_test ts_test_mt ts_calibrate ts_print ts_print_mt ts_print_raw ts_print_raw_mt ts_harvest ts_finddev

ts_test_SOURCES		= ts_test.c testutils.c testutils.h fbutils.c fbutils.h font_8x8.c font_8x16.c font.h
ts_test_LDADD		= $(top_builddir)/src/libts.la

ts_test_mt_SOURCES	= ts_test_mt.c testutils.c testutils.h fbutils.c fbutils.h font_8x8.c font_8x16.c font.h
ts_test_mt_LDADD	= $(top_builddir)/src/libts.la

ts_print_SOURCES	= ts_print.c
ts_print_LDADD		= $(top_builddir)/src/libts.la

ts_print_mt_SOURCES	= ts_print_mt.c
ts_print_mt_LDADD	= $(top_builddir)/src/libts.la

ts_print_raw_SOURCES	= ts_print_raw.c
ts_print_raw_LDADD	= $(top_builddir)/src/libts.la

ts_print_raw_mt_SOURCES	= ts_print_raw_mt.c
ts_print_raw_mt_LDADD	= $(top_builddir)/src/libts.la

ts_calibrate_SOURCES	= ts_calibrate.c fbutils.c fbutils.h testutils.c testutils.h font_8x8.c font_8x16.c font.h
ts_calibrate_LDADD	= $(top_builddir)/src/libts.la

ts_harvest_SOURCES	= ts_harvest.c fbutils.c fbutils.h testutils.c testutils.h font_8x8.c font_8x16.c font.h
ts_harvest_LDADD	= $(top_builddir)/src/libts.la

ts_finddev_SOURCES      = ts_finddev.c
ts_finddev_LDADD        = $(top_builddir)/src/libts.la
