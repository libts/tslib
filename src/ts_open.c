/*
 *  tslib/src/ts_open.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_open.c,v 1.3 2002/06/17 17:21:43 dlowder Exp $
 *
 * Open a touchscreen device.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/fcntl.h>

#ifdef USE_INPUT_API
#include <linux/input.h>
#endif /* USE_INPUT_API */

#include "tslib-private.h"

extern struct tslib_module_info __ts_raw;

struct tsdev *ts_open(const char *name, int nonblock)
{
	struct tsdev *ts;
	int flags = O_RDONLY;

	if (nonblock)
		flags |= O_NONBLOCK;

	ts = malloc(sizeof(struct tsdev));
	if (ts) {
#ifdef USE_INPUT_API
		int version;
		long bit;
#endif /* USE_INPUT_API */

		memset(ts, 0, sizeof(struct tsdev));

		ts->fd = open(name, flags);
		if (ts->fd == -1)
			goto free;

#ifdef USE_INPUT_API
		/* make sure we're dealing with a touchscreen device */
		if (ioctl(ts->fd, EVIOCGVERSION, &version) < 0 ||
		    version != EV_VERSION ||
		    ioctl(ts->fd, EVIOCGBIT(0, sizeof(bit)*8), &bit) < 0 ||
		    !(bit & (1 << EV_ABS)) ||
		    ioctl(ts->fd, EVIOCGBIT(EV_ABS, sizeof(bit)*8), &bit) < 0 ||
		    !(bit & (1 << ABS_X)) ||
		    !(bit & (1 << ABS_Y)) ||
		    !(bit & (1 << ABS_PRESSURE)))
			goto close;
#endif /* USE_INPUT_API */

		__ts_attach(ts, &__ts_raw);
	}

	return ts;

#ifdef USE_INPUT_API
close:
	close(ts->fd);
#endif /* USE_INPUT_API */

free:
	free(ts);
	return NULL;
}
