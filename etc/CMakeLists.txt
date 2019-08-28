#
#  tslib/etc/CMakeLists.txt
#
#  Copyright (C) 2018 Tuomo Rinne.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

set(tslib_extras ts.conf)

install(FILES ${tslib_extras} DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR})
