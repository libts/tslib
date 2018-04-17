/*
 *  tslib/src/ts_version.c
 *
 *  Copyright (C) 2017 Martin Kepplinger
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * This file contains libts version information only. Check the README.md
 * file for details.
 */
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "tslib.h"

#define LIBTS_DATESTAMP "[unreleased]"

static struct ts_lib_version_data version_data = {
	PACKAGE_VERSION,
	0x000000,	/* version_num calculated below */
	0		/* features */
	| TSLIB_VERSION_MT
	| TSLIB_VERSION_OPEN_RESTRICTED
	| TSLIB_VERSION_EVENTPATH
	| TSLIB_VERSION_VERSION
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

#ifdef LIBTS_VERSION_CURRENT
	major = LIBTS_VERSION_CURRENT - LIBTS_VERSION_AGE;
	minor = LIBTS_VERSION_AGE;
	patch = LIBTS_VERSION_REVISION;

	version_data.version_num |= major << 16;
	version_data.version_num |= minor << 8;
	version_data.version_num |= patch << 0;
#endif

	initialized = 1;

	return &version_data;
}

/* remember our library version value is 24 bit. 8 bit per library version */
char *tslib_version(void)
{
	static char version[100];
	struct ts_lib_version_data *ver = ts_libversion();
	static short initialized;

	if (initialized)
		return version;

	snprintf(version, sizeof(version),
		"tslib %s / libts ABI version %d (0x%06X)\nRelease-Date: %s\n",
		ver->package_version, ver->version_num >> 16, ver->version_num,
		LIBTS_DATESTAMP);

	initialized = 1;

	return version;
}

static void print_spaces(unsigned int nr)
{
	unsigned int i;

	for (i = 0; i < nr; i++) {
		printf(" ");
	}
}

void ts_print_ascii_logo(unsigned int pos)
{
	print_spaces(pos);
	printf(" _       _ _ _\n");
	print_spaces(pos);
	printf("| |_ ___| (_) |__\n");
	print_spaces(pos);
	printf("| __/ __| | | '_ \\\n");
	print_spaces(pos);
	printf("| |_\\__ \\ | | |_) |\n");
	print_spaces(pos);
	printf(" \\__|___/_|_|_.__/\n");
}
