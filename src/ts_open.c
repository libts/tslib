/*
 *  tslib/src/ts_open.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_open.c,v 1.2 2002/05/20 22:10:00 nico Exp $
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

#include <linux/input.h>

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
		int version;
		long bit;

		memset(ts, 0, sizeof(struct tsdev));

		ts->fd = open(name, flags);
		if (ts->fd == -1)
			goto free;

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

		__ts_attach(ts, &__ts_raw);
	}

	return ts;

close:
	close(ts->fd);
free:
	free(ts);
	return NULL;
}
