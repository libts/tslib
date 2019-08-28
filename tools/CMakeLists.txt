#
#  tslib/tools/CMakeLists.txt
#
#  Copyright (C) 2018 Tuomo Rinne.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

add_executable(ts_uinput ts_uinput.c)
target_link_libraries(ts_uinput tslib)
target_compile_definitions(ts_uinput PRIVATE TS_POINTERCAL="${TS_POINTERCAL}")

install(TARGETS ts_uinput
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
