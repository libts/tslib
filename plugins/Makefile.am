#
#  tslib/plugins/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
# $Id: Makefile.am,v 1.7 2005/02/28 23:44:12 kergoth Exp $
#

AM_CFLAGS		= $(DEBUGFLAGS) $(LIBFLAGS) $(VIS_CFLAGS)
LDADD		= -rpath $(PLUGIN_DIR)
INCLUDES		= -I$(top_srcdir)/src
  
#LTVSN			= -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
#			   -release $(LT_RELEASE)
LTVSN			= -avoid-version
LIBS			=
pluginexecdir		= $(PLUGIN_DIR)
  
if ENABLE_LINEAR_MODULE
LINEAR_MODULE = linear.la
else
LINEAR_MODULE =
endif

if ENABLE_LINEAR_MODULE
DEJITTER_MODULE = dejitter.la
else
DEJITTER_MODULE =
endif

if ENABLE_VARIANCE_MODULE
VARIANCE_MODULE = variance.la
else
VARIANCE_MODULE =
endif

if ENABLE_PTHRES_MODULE
PTHRES_MODULE = pthres.la
else
PTHRES_MODULE =
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

if ENABLE_INPUT_MODULE
INPUT_MODULE = input.la
else
INPUT_MODULE =
endif

if ENABLE_H2200_LINEAR_MODULE
H2200_LINEAR_MODULE = linear_h2200.la
else
H2200_LINEAR_MODULE =
endif

pluginexec_LTLIBRARIES = \
	$(LINEAR_MODULE) \
	$(DEJITTER_MODULE) \
	$(VARIANCE_MODULE) \
	$(PTHRES_MODULE) \
	$(UCB1X00_MODULE) \
	$(CORGI_MODULE) \
	$(COLLIE_MODULE) \
	$(H3600_MODULE) \
	$(MK712_MODULE) \
	$(ARCTIC2_MODULE) \
	$(H2200_LINEAR_MODULE) \
	$(INPUT_MODULE)
  
variance_la_SOURCES	= variance.c
variance_la_LDFLAGS	= -module $(LTVSN)
  
dejitter_la_SOURCES	= dejitter.c
dejitter_la_LDFLAGS	= -module $(LTVSN)
  
linear_la_SOURCES	= linear.c
linear_la_LDFLAGS	= -module $(LTVSN)

pthres_la_SOURCES	= pthres.c
pthres_la_LDFLAGS	= -module $(LTVSN)

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

input_la_SOURCES	= input-raw.c
input_la_LDFLAGS	= -module $(LTVSN)

linear_h2200_la_SOURCES	= linear-h2200.c
linear_h2200_la_LDFLAGS	= -module $(LTVSN)
