#
#  tslib/tests/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
# $Id: Makefile.am,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
#

INCLUDES		= -I$(top_srcdir)/src

bin_PROGRAMS		= ts_test ts_calibrate

ts_test_SOURCES		= ts_test.c
ts_test_LDADD		= $(top_builddir)/src/libts.la

ts_calibrate_SOURCES	= ts_calibrate.c
ts_calibrate_LDADD	= $(top_builddir)/src/libts.la
