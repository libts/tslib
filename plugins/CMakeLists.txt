#
#  tslib/plugins/CMakeLists.txt
#
#  Copyright (C) 2018 Tuomo Rinne.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#


macro(TSLIB_CHECK_MODULE module_name default_option help_string)
	option(enable-${module_name} ${help_string} ${default_option})
	option(static-${module_name} OFF)

	set(module_sources ${ARGN})

	if (${enable-${module_name}} OR ${static-${module_name}})
		if (${static-${module_name}})
			string(TOUPPER ${module_name} module_name_uc)
			string(REPLACE "-" "_" module_name_uc_ul ${module_name_uc})
			target_sources(tslib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${module_sources})
			target_compile_definitions(tslib PRIVATE TSLIB_STATIC_${module_name_uc_ul}_MODULE)
		else()
			add_library(${module_name} MODULE ${module_sources})
			list(APPEND plugin_targets ${module_name})
			target_link_libraries(${module_name} PUBLIC tslib)
			SET_TARGET_PROPERTIES(${module_name} PROPERTIES PREFIX "")
		endif()
	endif()

endmacro()

### filters
#########################

TSLIB_CHECK_MODULE(debounce ON "Enable building filter" debounce.c) 
TSLIB_CHECK_MODULE(dejitter ON "Enable building dejitter filter" dejitter.c) 
TSLIB_CHECK_MODULE(iir      ON "Enable building iir filter" iir.c) 
TSLIB_CHECK_MODULE(invert   ON "Enable building invert filter" invert.c) 
TSLIB_CHECK_MODULE(linear   ON "Enable building linear filter" linear.c) 
TSLIB_CHECK_MODULE(lowpass  ON "Enable building lowpass filter" lowpass.c) 
TSLIB_CHECK_MODULE(median   ON "Enable building median filter" median.c) 
TSLIB_CHECK_MODULE(pthres   ON "Enable building pthres filter" pthres.c) 
TSLIB_CHECK_MODULE(skip     ON "Enable building skip filter" skip.c) 
TSLIB_CHECK_MODULE(variance ON "Enable building variance filter" variance.c) 

# hardware access modules  
#########################

# generic, recommended
TSLIB_CHECK_MODULE(input             ON "Enable building of generic input raw module (Linux /dev/input/eventN support)" input-raw.c) 

# userspace device drivers, enabled by default (may become disabled by default in the future)
TSLIB_CHECK_MODULE(touchkit          ON "Enable building of serial TouchKit raw module (Linux /dev/ttySX support)" touchkit-raw.c) 
TSLIB_CHECK_MODULE(waveshare         ON "Enable building of WaveShare raw module (Linux /dev/hidrawN support)" waveshare-raw.c) 

# userspace device drivers, disabled by default
TSLIB_CHECK_MODULE(tatung            OFF "Enable building of tatung raw module (Tatung Webpad support)" tatung-raw.c) 
TSLIB_CHECK_MODULE(ucb1x00           OFF "Enable building of ucb1x00 raw module (UCB1x00 support)" ucb1x00-raw.c) 
TSLIB_CHECK_MODULE(mk712             OFF "Enable building of mk712 raw module (Hi tachi support)" mk712-raw.c) 
TSLIB_CHECK_MODULE(h3600             OFF "Enable building of h3600 raw module (HP iPaq H3600 support)" h3600-raw.c) 
TSLIB_CHECK_MODULE(dmc               OFF "Enable building of dmc raw module (HP iPaq DMC support)" dmc-raw.c) 
TSLIB_CHECK_MODULE(linear-h2200      OFF "Enable building of linearizing filter for iPAQ h2200" linear-h2200.c)
TSLIB_CHECK_MODULE(corgi             OFF "Enable building of corgi raw module (Sharp Zaurus sl-c7x0 support)" corgi-raw.c) 
TSLIB_CHECK_MODULE(collie            OFF "Enable building of collie raw module (Sharp Zaurus sl-5500/5000d support)" collie-raw.c) 
TSLIB_CHECK_MODULE(arctic2           OFF "Enable building of arctic2 raw module (IBM Arctic II support)" arctic2-raw.c) 
TSLIB_CHECK_MODULE(dmc_dus3000       OFF "Enable building of dmc_dus3000 raw module (DMC DUS Series support)" dmc_dus3000-raw.c) 
TSLIB_CHECK_MODULE(cy8mrln-palmpre   OFF "Enable building of cy8mrln-palmpre raw module" cy8mrln-palmpre.c)
TSLIB_CHECK_MODULE(galax             OFF "Enable building of HID USB eGalax raw module (Linux /dev/hiddevN support)" galax-raw.c) 
TSLIB_CHECK_MODULE(one-wire-ts-input OFF "Enable building of FriendlyARM one-wire raw module" one-wire-ts-input-raw.c)

#install plugins and add to export set
install(TARGETS ${plugin_targets} 
	OPTIONAL
	LIBRARY DESTINATION lib/ts)

