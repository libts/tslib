/*
 *  tslib/src/ts_open.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
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
	printf("%s ", tslib_version());

#if defined (__linux__)
	printf("Host OS: Linux");
#elif defined (__FreeBSD__)
	printf("Host OS: FreeBSD");
#elif defined (__OpenBSD__)
	printf("Host OS: OpenBSD");
#elif defined (__GNU__) && defined (__MACH__)
	printf("Host OS: Hurd");
#elif defined (__HAIKU__)
	printf("Host OS: Haiku");
#elif defined (__BEOS__)
	printf("Host OS: BeOS");
#elif defined (WIN32)
	printf("Host OS: Windows");
#elif defined (__APPLE__) && defined (__MACH__)
	printf("Host OS: Darwin");
#else
	printf("Host OS: unknown");
#endif
}
#endif /* DEBUG */

int (*ts_open_restricted)(const char *path, int flags, void *user_data) = NULL;

struct tsdev *ts_open(const char *name, int nonblock)
{
	struct tsdev *ts;
	int flags = O_RDWR;

#ifdef DEBUG
	print_host_os();
	printf(", trying to open %s\n", name);
#endif

	if (nonblock) {
	#ifndef WIN32
		flags |= O_NONBLOCK;
	#endif
	}

	ts = malloc(sizeof(struct tsdev));
	if (!ts)
		return NULL;

	memset(ts, 0, sizeof(struct tsdev));

	ts->eventpath = strdup(name);
	if (!ts->eventpath)
		goto free;

	if (ts_open_restricted) {
		ts->fd = ts_open_restricted(name, flags, NULL);
		if (ts->fd == -1)
			goto free;

		return ts;
	}

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

	return ts;

free:
	free(ts->eventpath);
	free(ts);
	return NULL;
}
