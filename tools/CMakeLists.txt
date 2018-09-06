#
#  tslib/tools/CMakeLists.txt
#
#  Copyright (C) 2018 Tuomo Rinne.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

option(ENABLE_TOOLS ON)

if (${ENABLE_TOOLS})
	add_executable(ts_uinput ts_uinput.c)
	target_link_libraries(ts_uinput tslib)
	target_compile_definitions(ts_uinput -DTS_POINTERCAL=${TS_POINTERCAL})

	install(TARGETS ts_uinput
		RUNTIME DESTINATION "bin")
endif()
