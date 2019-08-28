#
#  tslib/src/CMakeLists.txt
#
#  Copyright (C) 2018 Tuomo Rinne.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

include(CheckIncludeFile)
include(CheckFunctionExists)

if (NOT ${CMAKE_DL_LIBS} STREQUAL "")
	set(HAVE_LIBDL 1)
	set(PLUGIN_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/ts)
endif()

check_include_file(unistd.h  HAVE_UNISTD_H)
check_function_exists(strsep HAVE_STRSEP)

configure_file(../cmake/config.h.in config.h @ONLY)

set(tslib_core_src  ts_attach.c
		    ts_close.c
		    ts_config.c
		    ts_error.c
		    ts_fd.c
		    ts_get_eventpath.c
		    ts_load_module.c
		    ts_open.c
		    ts_option.c
		    ts_parse_vars.c
		    ts_read.c
		    ts_read_raw.c
		    ts_setup.c
		    ts_strsep.c
		    ts_version.c
)

add_library(tslib ${tslib_core_src})
set_property(TARGET tslib PROPERTY POSITION_INDEPENDENT_CODE ON)

math(EXPR LIBTS_MAJOR "${LIBTS_VERSION_CURRENT} - ${LIBTS_VERSION_AGE}")
set_target_properties(tslib PROPERTIES VERSION ${LIBTS_MAJOR}.${LIBTS_VERSION_AGE}.${LIBTS_VERSION_REVISION}
                                       SOVERSION ${LIBTS_MAJOR} )


if (WIN32)
	target_compile_definitions(tslib PRIVATE _CRT_SECURE_NO_WARNINGS)
endif (WIN32)

target_compile_definitions(tslib PUBLIC 
				TS_CONF="${TS_CONF}" 
				TS_POINTERCAL="${TS_POINTERCAL}"
				PLUGIN_DIR="${PLUGIN_DIR}"
				$<BUILD_INTERFACE:TSLIB_INTERNAL>)
target_link_libraries(tslib ${CMAKE_DL_LIBS})

target_include_directories(tslib PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}>"
				       	"$<INSTALL_INTERFACE:include>")

set_target_properties(tslib PROPERTIES PUBLIC_HEADER tslib.h)
set_target_properties(tslib PROPERTIES OUTPUT_NAME   ts)

install(TARGETS tslib EXPORT tslibTargets
           LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	   ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	   PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
