/*
 *  tslib/src/ts_read_raw.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_read_raw.c,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Read raw pressure, x, y, and timestamp from a touchscreen device.
 */
#include "config.h"

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/time.h>

#include "tslib-private.h"

struct ts_event {
	unsigned short	pressure;
	unsigned short	x;
	unsigned short	y;
	unsigned short	pad;
	struct timeval	stamp;
};

int ts_read_raw(struct tsdev *ts, struct ts_sample *samp, int nr)
{
	struct ts_event *evt;
	int ret;

	evt = alloca(sizeof(*evt) * nr);

	ret = read(ts->fd, evt, sizeof(*evt) * nr);

	if (ret >= 0) {
		int nr = ret / sizeof(*evt);
		while (ret >= sizeof(*evt)) {
			samp->x = evt->x;
			samp->y = evt->y;
			samp->pressure = evt->pressure;
			samp->tv.tv_usec = evt->stamp.tv_usec;
			samp->tv.tv_sec = evt->stamp.tv_sec;
			samp++;
			evt++;
			ret -= sizeof(*evt);
		}
		ret = nr;
	}

	return ret;
}

static int __ts_read_raw(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	return ts_read_raw(inf->dev, samp, nr);
}

static const struct tslib_ops __ts_raw_ops =
{
	read:	__ts_read_raw,
};

struct tslib_module_info __ts_raw =
{
	next:	NULL,
	ops:	&__ts_raw_ops,
};
