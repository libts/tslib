#
#  tslib/plugins/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

EXTRA_DIST		= CMakeLists.txt

AM_CFLAGS		= -DTS_POINTERCAL=\"@TS_POINTERCAL@\" $(DEBUGFLAGS) $(LIBFLAGS) $(VIS_CFLAGS)
LDADD			= -rpath $(PLUGIN_DIR)
AM_CPPFLAGS		= -I$(top_srcdir)/src
  
#LTVSN			= -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
LTVSN			= -avoid-version
pluginexecdir		= $(PLUGIN_DIR)
  
if ENABLE_LINEAR_MODULE
LINEAR_MODULE = linear.la
else
LINEAR_MODULE =
endif

if ENABLE_DEJITTER_MODULE
DEJITTER_MODULE = dejitter.la
else
DEJITTER_MODULE =
endif

if ENABLE_VARIANCE_MODULE
VARIANCE_MODULE = variance.la
else
VARIANCE_MODULE =
endif

if ENABLE_MEDIAN_MODULE
MEDIAN_MODULE = median.la
else
MEDIAN_MODULE =
endif

if ENABLE_PTHRES_MODULE
PTHRES_MODULE = pthres.la
else
PTHRES_MODULE =
endif

if ENABLE_DEBOUNCE_MODULE
DEBOUNCE_MODULE = debounce.la
else
DEBOUNCE_MODULE =
endif

if ENABLE_SKIP_MODULE
SKIP_MODULE = skip.la
else
SKIP_MODULE =
endif

if ENABLE_INVERT_MODULE
INVERT_MODULE = invert.la
else
INVERT_MODULE =
endif

if ENABLE_IIR_MODULE
IIR_MODULE = iir.la
else
IIR_MODULE =
endif

if ENABLE_EVTHRES_MODULE
EVTHRES_MODULE = evthres.la
else
EVTHRES_MODULE =
endif

if ENABLE_LOWPASS_MODULE
LOWPASS_MODULE = lowpass.la
else
LOWPASS_MODULE =
endif

if ENABLE_UCB1X00_MODULE
UCB1X00_MODULE = ucb1x00.la
else
UCB1X00_MODULE =
endif

if ENABLE_CORGI_MODULE
CORGI_MODULE = corgi.la
else
CORGI_MODULE =
endif

if ENABLE_COLLIE_MODULE
COLLIE_MODULE = collie.la
else
COLLIE_MODULE =
endif

if ENABLE_H3600_MODULE
H3600_MODULE = h3600.la
else
H3600_MODULE =
endif

if ENABLE_MK712_MODULE
MK712_MODULE = mk712.la
else
MK712_MODULE =
endif

if ENABLE_ARCTIC2_MODULE
ARCTIC2_MODULE = arctic2.la
else
ARCTIC2_MODULE =
endif

if ENABLE_TATUNG_MODULE
TATUNG_MODULE = tatung.la
else
TATUNG_MODULE =
endif

if ENABLE_DMC_MODULE
DMC_MODULE = dmc.la
else
DMC_MODULE =
endif

if ENABLE_DMC_DUS3000_MODULE
DMC_DUS3000_MODULE = dmc_dus3000.la
else
DMC_DUS3000_MODULE =
endif

if ENABLE_INPUT_MODULE
INPUT_MODULE = input.la
else
INPUT_MODULE =
endif

if ENABLE_INPUT_EVDEV_MODULE
INPUT_EVDEV_MODULE = input_evdev.la
else
INPUT_EVDEV_MODULE =
endif

if ENABLE_GALAX_MODULE
GALAX_MODULE = galax.la
else
GALAX_MODULE =
endif


if ENABLE_TOUCHKIT_MODULE
TOUCHKIT_MODULE = touchkit.la
else
TOUCHKIT_MODULE =
endif

if ENABLE_LINEAR_H2200_MODULE
H2200_LINEAR_MODULE = linear_h2200.la
else
H2200_LINEAR_MODULE =
endif

if ENABLE_CY8MRLN_PALMPRE_MODULE
CY8MRLN_PALMPRE_MODULE = cy8mrln_palmpre.la
else
CY8MRLN_PALMPRE_MODULE =
endif

if ENABLE_WAVESHARE_MODULE
WAVESHARE_MODULE = waveshare.la
else
WAVESHARE_MODULE =
endif

if ENABLE_ONE_WIRE_TS_INPUT_MODULE
ONE_WIRE_TS_INPUT_MODULE = one_wire_ts_input.la
else
ONE_WIRE_TS_INPUT_MODULE =
endif

pluginexec_LTLIBRARIES = \
	$(LINEAR_MODULE) \
	$(DEJITTER_MODULE) \
	$(VARIANCE_MODULE) \
	$(MEDIAN_MODULE) \
	$(PTHRES_MODULE) \
	$(DEBOUNCE_MODULE) \
	$(SKIP_MODULE) \
	$(INVERT_MODULE) \
	$(IIR_MODULE) \
	$(EVTHRES_MODULE) \
	$(LOWPASS_MODULE) \
	$(UCB1X00_MODULE) \
	$(CORGI_MODULE) \
	$(COLLIE_MODULE) \
	$(H3600_MODULE) \
	$(MK712_MODULE) \
	$(ARCTIC2_MODULE) \
	$(TATUNG_MODULE) \
	$(DMC_MODULE) \
	$(DMC_DUS3000_MODULE) \
	$(H2200_LINEAR_MODULE) \
	$(INPUT_MODULE) \
	$(INPUT_EVDEV_MODULE) \
	$(GALAX_MODULE) \
	$(TOUCHKIT_MODULE) \
	$(CY8MRLN_PALMPRE_MODULE) \
	$(ONE_WIRE_TS_INPUT_MODULE) \
	$(WAVESHARE_MODULE)
  
variance_la_SOURCES	= variance.c
variance_la_LDFLAGS	= -module $(LTVSN)
if WINDOWS
variance_la_LDFLAGS	+= -no-undefined
endif
variance_la_LIBADD	= $(top_builddir)/src/libts.la
  
median_la_SOURCES	= median.c
median_la_LDFLAGS	= -module $(LTVSN)
if WINDOWS
median_la_LDFLAGS	+= -no-undefined
endif
median_la_LIBADD	= $(top_builddir)/src/libts.la

dejitter_la_SOURCES	= dejitter.c
dejitter_la_LDFLAGS	= -module $(LTVSN)
if WINDOWS
dejitter_la_LDFLAGS	+= -no-undefined
endif
dejitter_la_LIBADD	= $(top_builddir)/src/libts.la
  
linear_la_SOURCES	= linear.c
linear_la_LDFLAGS	= -module $(LTVSN)
if WINDOWS
linear_la_LDFLAGS	+= -no-undefined
endif
linear_la_LIBADD	= $(top_builddir)/src/libts.la

pthres_la_SOURCES	= pthres.c
pthres_la_LDFLAGS	= -module $(LTVSN)
if WINDOWS
pthres_la_LDFLAGS	+= -no-undefined
endif
pthres_la_LIBADD	= $(top_builddir)/src/libts.la

debounce_la_SOURCES	= debounce.c
debounce_la_LDFLAGS	= -module $(LTVSN)
if WINDOWS
debounce_la_LDFLAGS	+= -no-undefined
endif
debounce_la_LIBADD	= $(top_builddir)/src/libts.la

skip_la_SOURCES		= skip.c
skip_la_LDFLAGS		= -module $(LTVSN)
if WINDOWS
skip_la_LDFLAGS		+= -no-undefined
endif
skip_la_LIBADD		= $(top_builddir)/src/libts.la

linear_h2200_la_SOURCES	= linear-h2200.c
linear_h2200_la_LDFLAGS	= -module $(LTVSN)
if WINDOWS
linear_h2200_la_LDFLAGS	+= -no-undefined
endif
linear_h2200_la_LIBADD	= $(top_builddir)/src/libts.la

invert_la_SOURCES	= invert.c
invert_la_LDFLAGS	= -module $(LTVSN)
if WINDOWS
invert_la_LDFLAGS	+= -no-undefined
endif
invert_la_LIBADD	= $(top_builddir)/src/libts.la

iir_la_SOURCES		= iir.c
iir_la_LDFLAGS		= -module $(LTVSN)
if WINDOWS
iir_la_LDFLAGS		+= -no-undefined
endif
iir_la_LIBADD		= $(top_builddir)/src/libts.la

evthres_la_SOURCES	= evthres.c
evthres_la_LDFLAGS	= -module $(LTVSN)
if WINDOWS
evthres_la_LDFLAGS	+= -no-undefined
endif
evthres_la_LIBADD	= $(top_builddir)/src/libts.la

lowpass_la_SOURCES	= lowpass.c
lowpass_la_LDFLAGS	= -module $(LTVSN)
if WINDOWS
lowpass_la_LDFLAGS	+= -no-undefined
endif
lowpass_la_LIBADD	= $(top_builddir)/src/libts.la

# hw access
corgi_la_SOURCES	= corgi-raw.c
corgi_la_LDFLAGS	= -module $(LTVSN)

collie_la_SOURCES	= collie-raw.c
collie_la_LDFLAGS	= -module $(LTVSN)

ucb1x00_la_SOURCES	= ucb1x00-raw.c
ucb1x00_la_LDFLAGS	= -module $(LTVSN)

h3600_la_SOURCES	= h3600-raw.c
h3600_la_LDFLAGS	= -module $(LTVSN)

mk712_la_SOURCES	= mk712-raw.c
mk712_la_LDFLAGS	= -module $(LTVSN)

arctic2_la_SOURCES	= arctic2-raw.c
arctic2_la_LDFLAGS	= -module $(LTVSN)

tatung_la_SOURCES	= tatung-raw.c
tatung_la_LDFLAGS	= -module $(LTVSN)

dmc_la_SOURCES		= dmc-raw.c
dmc_la_LDFLAGS		= -module $(LTVSN)

dmc_dus3000_la_SOURCES	= dmc_dus3000-raw.c
dmc_dus3000_la_LDFLAGS	= -module $(LTVSN)

input_la_SOURCES	= input-raw.c
input_la_LDFLAGS	= -module $(LTVSN)
input_la_LIBADD		= $(top_builddir)/src/libts.la

input_evdev_la_SOURCES	= input-evdev-raw.c
input_evdev_la_LDFLAGS	= -module $(LTVSN)
input_evdev_la_LIBADD	= $(top_builddir)/src/libts.la $(LIBEVDEV_LIBS)
input_evdev_la_CFLAGS	= $(LIBEVDEV_CFLAGS)

galax_la_SOURCES = galax-raw.c
galax_la_LDFLAGS = -module $(LTVSN)
galax_la_LIBADD = $(top_builddir)/src/libts.la

touchkit_la_SOURCES	= touchkit-raw.c
touchkit_la_LDFLAGS	= -module $(LTVSN)

cy8mrln_palmpre_la_SOURCES = cy8mrln-palmpre.c cy8mrln.h
cy8mrln_palmpre_la_LDFLAGS = -module $(LTVSN)
cy8mrln_palmpre_la_LIBADD  = $(top_builddir)/src/libts.la

waveshare_la_SOURCES	= waveshare-raw.c
waveshare_la_LDFLAGS	= -module $(LTVSN)
waveshare_la_LIBADD	= $(top_builddir)/src/libts.la

one_wire_ts_input_la_SOURCES	= one-wire-ts-input-raw.c
one_wire_ts_input_la_LDFLAGS	= -module $(LTVSN)
one_wire_ts_input_la_LIBADD	= $(top_builddir)/src/libts.la
