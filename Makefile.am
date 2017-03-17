#
#  tslib/Makefile.am
#
#  Copyright (C) 2017 Martin Kepplinger <martink@posteo.de>
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

SUBDIRS		= etc src plugins tests tools doc
EXTRA_DIST	= autogen.sh README.md m4/external
AM_DISTCHECK_CONFIGURE_FLAGS = --disable-dependency-tracking
DISTCLEANFILES = $(PACKAGE).pc

ACLOCAL_AMFLAGS = -I m4/internal -I m4/external
AUTOMAKE_OPTIONS = 1.6

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(PACKAGE).pc
