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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/fcntl.h>

#include "tslib-private.h"

extern struct tslib_module_info __ts_raw;

struct tsdev *ts_open(const char *name, int nonblock)
{
	struct tsdev *ts;
	int flags = O_RDWR;

	if (nonblock)
		flags |= O_NONBLOCK;

	ts = malloc(sizeof(struct tsdev));
	if (ts) {
		memset(ts, 0, sizeof(struct tsdev));

		ts->fd = open(name, flags);
		/*
		 * Try again in case file is simply not writable
		 * It will do for most drivers
		 */
		if (ts->fd == -1 && errno == EACCES) {
			flags = nonblock ? (O_RDONLY | O_NONBLOCK) : O_RDONLY;
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
