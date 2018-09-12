#
#  tslib/tools/Makefile.am
#
#  Copyright (C) 2016 Martin Kepplinger
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

if ENABLE_TOOLS
if LINUX
EXTRA_DIST		= ts_uinput_start.sh CMakeLists.txt
AM_CFLAGS               = -DTS_POINTERCAL=\"@TS_POINTERCAL@\" $(DEBUGFLAGS)
AM_CPPFLAGS		= -I$(top_srcdir)/src

bin_PROGRAMS		= ts_uinput

ts_uinput_SOURCES	= ts_uinput.c
ts_uinput_LDADD		= $(top_builddir)/src/libts.la $(LIBEVDEV_LIBS)
endif
endif
