#
#  tslib/CMakeLists.txt
#
#  Copyright (C) 2018 Tuomo Rinne.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

cmake_minimum_required(VERSION 3.1)

cmake_policy(SET CMP0057 NEW)

project(tslib)

option(BUILD_SHARED_LIBS "ON:  tslib is build as shared; 
			  OFF: tslib is build as static" ON)
set(LIBTS_VERSION_CURRENT 10)
set(LIBTS_VERSION_REVISION 0)
set(LIBTS_VERSION_AGE 10)

set(PACKAGE_VERSION 1.18-rc1)
set(tslib_extras_dir ${CMAKE_INSTALL_PREFIX}/etc)
set(TS_POINTERCAL ${tslib_extras_dir}/pointercal)
set(TS_CONF ${tslib_extras_dir}/ts.conf)

add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(tests)
add_subdirectory(tools)
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
  INSTALL_DESTINATION lib/cmake/tslib)

install(EXPORT tslibTargets
  FILE tslibTargets.cmake
  NAMESPACE tslib::
  DESTINATION lib/cmake/tslib)

install(FILES 
  ${CMAKE_CURRENT_BINARY_DIR}/tslibConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/tslibConfigVersion.cmake
  DESTINATION lib/cmake/tslib)
