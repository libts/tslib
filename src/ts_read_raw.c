/*
 *  tslib/src/ts_read_raw.c
 *
 *  Copyright (C) 2003 Chris Larson.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
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

int ts_read_raw(struct tsdev *ts, struct ts_sample *samp, int nr)
{
#ifdef DEBUG
	int i;
#endif
	int result = ts->list_raw->ops->read(ts->list_raw, samp, nr);

#ifdef DEBUG
	for (i = 0; i < result; i++) {
		fprintf(stderr,
			"TS_READ_RAW----> x = %d, y = %d, pressure = %d\n",
			samp->x, samp->y, samp->pressure);

		samp++;
	}
#endif
	return result;
}

int ts_read_raw_mt(struct tsdev *ts, struct ts_sample_mt **samp, int slots,
		   int nr)
{
#ifdef DEBUG
	int i, j;
#endif

	int result = ts->list_raw->ops->read_mt(ts->list_raw, samp, slots, nr);
#ifdef DEBUG
	for (i = 0; i < result; i++) {
		for (j = 0; j < slots; j++) {
			if (!(samp[i][j].valid & TSLIB_MT_VALID))
				continue;

			ts_error("TS_READ_RAW_MT----> slot %d: x = %d, y = %d, pressure = %d\n",
				 samp[i][j].slot, samp[i][j].x, samp[i][j].y,
				 samp[i][j].pressure);
		}
	}
#endif
	return result;
}
