#!/bin/sh
# $Id: autogen.sh,v 1.2 2002/08/29 20:40:09 dlowder Exp $
echo -n "Libtoolize..."
libtoolize --force --copy
echo "Done."
echo -n "Aclocal..."
aclocal
echo "Done."
echo -n "Autoheader..."
autoheader
echo "Done."
echo -n "Automake..."
automake --add-missing --copy
echo "Done."
echo -n "Autoconf..."
autoconf
echo "Done."
#./configure $*
echo "Now you can do ./configure, make, make install."
