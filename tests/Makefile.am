#
#  tslib/tests/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
# $Id: Makefile.am,v 1.3 2002/08/29 20:40:09 dlowder Exp $
#

CFLAGS                  :=  $(CFLAGS) $(DEBUGFLAGS) $(INPUTAPIFLAG)
INCLUDES		= -I$(top_srcdir)/src

bin_PROGRAMS		= ts_test ts_calibrate ts_print ts_print_raw

ts_test_SOURCES		= ts_test.c fbutils.c font_8x8.c font_8x16.c
ts_test_LDADD		= $(top_builddir)/src/libts.la

ts_print_SOURCES	= ts_print.c
ts_print_LDADD		= $(top_builddir)/src/libts.la

ts_print_raw_SOURCES	= ts_print_raw.c
ts_print_raw_LDADD	= $(top_builddir)/src/libts.la

ts_calibrate_SOURCES	= ts_calibrate.c fbutils.c font_8x8.c font_8x16.c
ts_calibrate_LDADD	= $(top_builddir)/src/libts.la
