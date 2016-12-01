#
#  tslib/src/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

AM_CFLAGS	 = -DPLUGIN_DIR=\"@PLUGIN_DIR@\" -DTS_CONF=\"@TS_CONF@\" -DTS_POINTERCAL=\"@TS_POINTERCAL@\" \
		   $(DEBUGFLAGS) $(LIBFLAGS) $(VIS_CFLAGS)

noinst_HEADERS   = tslib-private.h tslib-filter.h
include_HEADERS  = tslib.h

lib_LTLIBRARIES  = libts.la
libts_la_SOURCES = ts_attach.c ts_close.c ts_config.c ts_error.c \
		   ts_fd.c ts_load_module.c ts_open.c ts_parse_vars.c \
		   ts_read.c ts_read_raw.c ts_option.c $(srcdir)/../plugins/plugins.h

if ENABLE_STATIC_LINEAR_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/linear.c
endif

if ENABLE_STATIC_DEJITTER_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/dejitter.c
endif

if ENABLE_STATIC_LINEAR_H2200_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/linear-h2200.c
endif

if ENABLE_STATIC_VARIANCE_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/variance.c
endif

if ENABLE_STATIC_MEDIAN_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/median.c
endif

if ENABLE_STATIC_PTHRES_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/pthres.c
endif

if ENABLE_STATIC_UCB1X00_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/ucb1x00-raw.c
endif

if ENABLE_STATIC_CORGI_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/corgi-raw.c
endif

if ENABLE_STATIC_COLLIE_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/collie-raw.c
endif

if ENABLE_STATIC_H3600_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/h3600-raw.c
endif

if ENABLE_STATIC_MK712_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/mk712-raw.c
endif

if ENABLE_STATIC_ARCTIC2_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/arctic2-raw.c
endif

if ENABLE_STATIC_TATUNG_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/tatung-raw.c
endif

if ENABLE_STATIC_INPUT_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/input-raw.c
endif

if ENABLE_STATIC_DEBOUNCE_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/debounce.c
endif

if ENABLE_STATIC_SKIP_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/skip.c
endif

if ENABLE_STATIC_WAVESHARE_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/waveshare-raw.c
endif

libts_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
		   -release $(LT_RELEASE) -export-dynamic
