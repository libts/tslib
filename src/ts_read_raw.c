/*
 *  tslib/src/ts_read_raw.c
 *
 *  Copyright (C) 2003 Chris Larson.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * Read raw pressure, x, y, and timestamp from a touchscreen device.
 */
#include "config.h"

#include "tslib-private.h"

#ifdef DEBUG
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

int ts_read_raw(struct tsdev *ts, struct ts_sample *samp, int nr)
{
	int result = ts->list_raw->ops->read(ts->list_raw, samp, nr);
#ifdef DEBUG
	fprintf(stderr,"TS_READ_RAW----> x = %d, y = %d, pressure = %d\n", samp->x, samp->y, samp->pressure);
#endif
	return result;
}
