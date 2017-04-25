/*
 *  tslib/src/ts_version.c
 *
 *  Copyright (C) 2017 Martin Kepplinger
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * This file contains libts version information only. Check the README.md
 * file for details.
 */
#include <stdlib.h>
#include "config.h"
#include "tslib.h"

static struct ts_lib_version_data version_data = {
	0,		/* age of this struct */
	PACKAGE_VERSION,
	0x000000,	/* version_num calculated below */
	0		/* features */
	| TSLIB_VERSION_MT
	,
};

/* This function returns a pointer to a static copy of the version info
 * struct.
 */
struct ts_lib_version_data *ts_libversion(void)
{
	static short initialized;
	int major = 0;
	int minor = 0;
	int patch = 0;

	if (initialized)
		return &version_data;

	major = LIBTS_VERSION_CURRENT - LIBTS_VERSION_AGE;
	minor = LIBTS_VERSION_AGE;
	patch = LIBTS_VERSION_REVISION;

	if (major == 0 && minor == 0)
		return NULL;

	version_data.version_num |= major << 16;
	version_data.version_num |= minor << 8;
	version_data.version_num |= patch << 0;

	initialized = 1;

	return &version_data;
}
