#
#  tslib/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
# $Id: Makefile.am,v 1.3 2005/02/28 18:56:09 kergoth Exp $
#

SUBDIRS		= etc src plugins tests
EXTRA_DIST	= autogen.sh

ACLOCAL_AMFLAGS = -I m4/internal -I m4/external
AUTOMAKE_OPTIONS = 1.6

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(PACKAGE)-$(LT_RELEASE).pc
