#
#  tslib/tests/CMakeLists.txt
#
#  Copyright (C) 2018 Tuomo Rinne.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#


option(with-sdl "build graphical tools like ts_calibrate using SDL" OFF)

if (${with-sdl})
	find_package(SDL2 REQUIRED)
endif()

function(TSLIB_ADD_TEST_ON_PLATFORMS test_name)
	foreach(condition IN LISTS ARGN)
		if (${condition})
			add_executable(${test_name} ${${test_name}_SOURCES})
			target_link_libraries(${test_name} PUBLIC tslib)
			if (${with-sdl})
				target_link_libraries(${test_name} PUBLIC SDL2)
			endif()
			set(tslib_tests ${tslib_tests} ${test_name} PARENT_SCOPE)
			break()
		endif()
	endforeach()
endfunction(TSLIB_ADD_TEST_ON_PLATFORMS)


set(fbutils $<$<PLATFORM_ID:FreeBSD>:fbutils-bsd.c>
	    $<$<PLATFORM_ID:Linux>:fbutils-linux.c>)


if(${with-sdl})
	set(ts_test_mt_SOURCES ts_test_mt_sdl.c sdlutils.c)
	set(ts_calibrate_SOURCES ts_calibrate_sdl.c ts_calibrate_common.c sdlutils.c)
else()
	set(ts_test_mt_SOURCES ts_test_mt.c testutils.c font_8x8.c font_8x16.c ${fbutils})
	set(ts_calibrate_SOURCES ts_calibrate.c ts_calibrate_common.c testutils.c font_8x8.c font_8x16.c ${fbutils})
endif()

set(ts_conf_SOURCES ts_conf.c)
set(ts_test_SOURCES ts_test.c testutils.c font_8x8.c font_8x16.c ${fbutils})
set(ts_print_SOURCES  ts_print.c)
set(ts_print_mt_SOURCES ts_print_mt.c)
set(ts_verify_SOURCES ts_verify.c)
set(ts_print_raw_SOURCES ts_print_raw.c)
set(ts_finddev_SOURCES ts_finddev.c)
set(ts_harvest_SOURCES ts_harvest.c testutils.c font_8x8.c font_8x16.c ${fbutils})


## ADD TESTS
########################
if (WIN32 AND ${with-sdl})
	set(WIN32_WITH_SDL TRUE)
endif()

if (UNIX AND NOT ${with-sdl})
	set(UNIX_WITHOUT_SDL TRUE)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
	set(LINUX TRUE)
endif()

TSLIB_ADD_TEST_ON_PLATFORMS(ts_conf      UNIX WIN32)
TSLIB_ADD_TEST_ON_PLATFORMS(ts_print     UNIX WIN32)
TSLIB_ADD_TEST_ON_PLATFORMS(ts_print_raw UNIX WIN32)
TSLIB_ADD_TEST_ON_PLATFORMS(ts_print_mt  UNIX WIN32)
TSLIB_ADD_TEST_ON_PLATFORMS(ts_harvest   UNIX)
TSLIB_ADD_TEST_ON_PLATFORMS(ts_finddev   UNIX)
TSLIB_ADD_TEST_ON_PLATFORMS(ts_test_mt   UNIX ${WIN32_WITH_SDL})
TSLIB_ADD_TEST_ON_PLATFORMS(ts_calibrate UNIX ${WIN32_WITH_SDL})
TSLIB_ADD_TEST_ON_PLATFORMS(ts_test   	 ${UNIX_WITHOUT_SDL})
TSLIB_ADD_TEST_ON_PLATFORMS(ts_verify    ${LINUX})

install(TARGETS ${tslib_tests}
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
