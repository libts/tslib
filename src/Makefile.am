#
#  tslib/src/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
# $Id: Makefile.am,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
#

CFLAGS		:= $(CFLAGS) -DPLUGIN_DIR=\"@PLUGIN_DIR@\" -DTS_CONF=\"@TS_CONF@\"

noinst_HEADERS   = tslib-private.h tslib-filter.h
include_HEADERS  = tslib.h

lib_LTLIBRARIES  = libts.la
libts_la_SOURCES = ts_attach.c ts_close.c ts_config.c ts_error.c \
		   ts_load_module.c ts_open.c ts_parse_vars.c    \
		   ts_read.c ts_read_raw.c
libts_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
		   -release $(LT_RELEASE) -export-dynamic
libts_la_LIBADD  = -ldl
