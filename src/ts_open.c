/*
 *  tslib/src/ts_open.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Open a touchscreen device.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "tslib-private.h"

#ifdef DEBUG
#include <stdio.h>
#endif

extern struct tslib_module_info __ts_raw;

#ifdef DEBUG
/* verify we use the correct OS specific code at runtime.
 * If we use it, test it here!
 */
static void print_host_os(void)
{
	struct ts_lib_version_data *version = ts_libversion();

	printf("tslib %s (library 0x%X) on ",
	       version->package_version, version->version_num);

#if defined (__linux__)
	printf("Host OS: Linux\n");
#elif defined (__FreeBSD__)
	printf("Host OS: FreeBSD\n");
#elif defined (__OpenBSD__)
	printf("Host OS: OpenBSD\n");
#elif defined (__GNU__) && defined (__MACH__)
	printf("Host OS: Hurd\n");
#elif defined (__HAIKU__)
	printf("Host OS: Haiku\n");
#elif defined (__BEOS__)
	printf("Host OS: BeOS\n");
#elif defined (WIN32)
	printf("Host OS: Windows\n");
#elif defined (__APPLE__) && defined (__MACH__)
	printf("Host OS: Darwin\n");
#else
	printf("Host OS: unknown\n");
#endif
}
#endif /* DEBUG */

struct tsdev *ts_open(const char *name, int nonblock)
{
	struct tsdev *ts;
	int flags = O_RDWR;

#ifdef DEBUG
	print_host_os();
#endif

	if (nonblock) {
	#ifndef WIN32
		flags |= O_NONBLOCK;
	#endif
	}

	ts = malloc(sizeof(struct tsdev));
	if (ts) {
		memset(ts, 0, sizeof(struct tsdev));

		ts->fd = open(name, flags);
		/*
		 * Try again in case file is simply not writable
		 * It will do for most drivers
		 */
		if (ts->fd == -1 && errno == EACCES) {
		#ifndef WIN32
			flags = nonblock ? (O_RDONLY | O_NONBLOCK) : O_RDONLY;
		#else
			flags = O_RDONLY;
		#endif
			ts->fd = open(name, flags);
		}
		if (ts->fd == -1)
			goto free;
	}

	return ts;

free:
	free(ts);
	return NULL;
}
