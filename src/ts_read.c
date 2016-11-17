/*
 *  tslib/src/ts_read.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
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

int ts_read(struct tsdev *ts, struct ts_sample *samp, int nr)
{
	int result;
#ifdef DEBUG
	int i;
#endif

	result = ts->list->ops->read(ts->list, samp, nr);
#ifdef DEBUG
	for (i = 0; i < result; i++) {
		fprintf(stderr,"TS_READ----> x = %d, y = %d, pressure = %d\n",
			samp->x, samp->y, samp->pressure);

		samp++;
	}
#endif
	return result;

}
