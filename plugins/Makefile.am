#
#  tslib/plugins/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
# $Id: Makefile.am,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
#

INCLUDES		 = -I$(top_srcdir)/src
LDFLAGS			:= $(LDFLAGS) -rpath $(PLUGIN_DIR)
#LTVSN			:= -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
#			   -release $(LT_RELEASE)
LTVSN			:= -avoid-version
LIBS			 =

plugindir		 = $(PLUGIN_DIR)

EXTRA_LTLIBRARIES	 = variance.la dejitter.la linear.la
plugin_LTLIBRARIES	 = $(PLUGINS)

variance_la_SOURCES	 = variance.c
variance_la_LDFLAGS	 = -module $(LTVSN)

dejitter_la_SOURCES	 = dejitter.c
dejitter_la_LDFLAGS	 = -module $(LTVSN)

linear_la_SOURCES	 = linear.c
linear_la_LDFLAGS	 = -module $(LTVSN)

