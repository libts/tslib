#!/bin/sh
# $Id: autogen.sh,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
libtoolize --force --copy
aclocal
autoheader
automake --add-missing --copy
autoconf
./configure $*
