#
#  tslib/doc/CMakeLists.txt
#
#  Copyright (C) 2018 Tuomo Rinne.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

set(tslib_general_man   ts_test.1 
	          	ts_test_mt.1 
	          	ts_calibrate.1 
	          	ts_uinput.1 
	      	  	ts_print.1 
	      	  	ts_print_raw.1 
	      	  	ts_print_mt.1 
	      	  	ts_finddev.1 
			ts_harvest.1
	      		ts_verify.1)


set(tslib_library_man   ts_read.3
			ts_read_mt.3 
			ts_read_raw.3 
			ts_read_raw_mt.3 
			ts_open.3 
			ts_close.3 
			ts_config.3
		       	ts_setup.3 
			ts_libversion.3 
			ts_fd.3 
			ts_error_fn.3 
			ts_open_restricted.3 
			ts_close_restricted.3 
			ts_get_eventpath.3)

set(tslib_misc_man      ts.conf.5)

install(FILES ${tslib_general_man} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
install(FILES ${tslib_library_man} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man3)
install(FILES ${tslib_misc_man}    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man5)
