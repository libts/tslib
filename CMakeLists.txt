#
#  tslib/CMakeLists.txt
#
#  Copyright (C) 2018 Tuomo Rinne.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

cmake_minimum_required(VERSION 3.10)

project(tslib LANGUAGES C)

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "ON:  tslib is build as shared; 
			  OFF: tslib is build as static" ON)
option(ENABLE_TOOLS "build additional tools" ON)

set(LIBTS_VERSION_CURRENT 10)
set(LIBTS_VERSION_REVISION 5)
set(LIBTS_VERSION_AGE 10)

set(PACKAGE_VERSION 1.23+)
set(TS_POINTERCAL ${CMAKE_INSTALL_FULL_SYSCONFDIR}/pointercal)
set(TS_CONF ${CMAKE_INSTALL_FULL_SYSCONFDIR}/ts.conf)

add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(tests)
if (ENABLE_TOOLS)
	add_subdirectory(tools)
endif ()
add_subdirectory(etc)
add_subdirectory(doc)

#create Config files to help other lbiraries to find the package
include(CMakePackageConfigHelpers)
set(config_package_location cmake)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/tslibConfigVersion.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(cmake/tslibConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/tslibConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tslib)

install(EXPORT tslibTargets
  FILE tslibTargets.cmake
  NAMESPACE tslib::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tslib)

install(FILES 
  ${CMAKE_CURRENT_BINARY_DIR}/tslibConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/tslibConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tslib)

# this is done in a function to have a local scope for the additional variables
function(pkgc_setup_file)
  set(prefix "${CMAKE_INSTALL_PREFIX}")
  set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
  set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
  set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
  set(PACKAGE tslib)
  set(PACKAGE_DESCRIPTION "Touchscreen Access Library")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tslib.pc.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/tslib.pc"
                 @ONLY)
endfunction()

pkgc_setup_file()
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tslib.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
