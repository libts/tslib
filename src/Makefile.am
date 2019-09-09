#
#  tslib/src/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

EXTRA_DIST	 = CMakeLists.txt

AM_CFLAGS	 = -DPLUGIN_DIR=\"@PLUGIN_DIR@\" -DTS_CONF=\"@TS_CONF@\" \
		   -DTS_POINTERCAL=\"@TS_POINTERCAL@\" \
		   $(DEBUGFLAGS) $(LIBFLAGS) $(VIS_CFLAGS) \
		   $(LIBEVDEV_CFLAGS)

noinst_HEADERS   = tslib-private.h tslib-filter.h
include_HEADERS  = tslib.h

lib_LTLIBRARIES  = libts.la
libts_la_SOURCES = ts_attach.c ts_close.c ts_config.c ts_error.c \
		   ts_fd.c ts_load_module.c ts_open.c ts_parse_vars.c \
		   ts_read.c ts_read_raw.c ts_option.c ts_setup.c \
		   $(srcdir)/../plugins/plugins.h ts_version.c \
		   ts_config_filter.c \
		   ts_get_eventpath.c

if !HAVE_STRSEP
libts_la_SOURCES += ts_strsep.c ts_strsep.h
endif

##################
# filter modules #
##################

if ENABLE_STATIC_DEBOUNCE_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/debounce.c
endif

if ENABLE_STATIC_DEJITTER_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/dejitter.c
endif

if ENABLE_STATIC_EVTHRES_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/evthres.c
endif

if ENABLE_STATIC_IIR_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/iir.c
endif

if ENABLE_STATIC_LINEAR_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/linear.c
endif

if ENABLE_STATIC_LINEAR_H2200_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/linear-h2200.c
endif

if ENABLE_STATIC_MEDIAN_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/median.c
endif

if ENABLE_STATIC_PTHRES_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/pthres.c
endif

if ENABLE_STATIC_SKIP_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/skip.c
endif

if ENABLE_STATIC_VARIANCE_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/variance.c
endif

if ENABLE_STATIC_INVERT_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/invert.c
endif

if ENABLE_STATIC_LOWPASS_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/lowpass.c
endif

###############
# raw modules #
###############

if ENABLE_STATIC_INPUT_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/input-raw.c
endif

if ENABLE_STATIC_INPUT_EVDEV_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/input-evdev-raw.c
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

if ENABLE_STATIC_DMC_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/dmc-raw.c
endif

if ENABLE_STATIC_TOUCHKIT_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/touchkit-raw.c
endif

if ENABLE_STATIC_WAVESHARE_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/waveshare-raw.c
endif

if ENABLE_STATIC_ONE_WIRE_TS_INPUT_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/one-wire-ts-input-raw.c
endif

if ENABLE_STATIC_DMC_DUS3000_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/dmc_dus3000-raw.c
endif

if ENABLE_STATIC_GALAX_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/galax-raw.c
endif

if ENABLE_STATIC_CY8MRLN_PALMPRE_MODULE
libts_la_SOURCES += $(top_srcdir)/plugins/cy8mrln.h
libts_la_SOURCES += $(top_srcdir)/plugins/cy8mrln-palmpre.c
endif

libts_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
		   -export-dynamic
if WINDOWS
libts_la_LDFLAGS += -no-undefined
endif
