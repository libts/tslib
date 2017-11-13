/*
 *  tslib/src/ts_read.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Read touch samples as tslib sample structs
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
		fprintf(stderr, "TS_READ----> x = %d, y = %d, pressure = %d\n",
			samp->x, samp->y, samp->pressure);

		samp++;
	}
#endif
	return result;

}

int ts_read_mt(struct tsdev *ts, struct ts_sample_mt **samp, int max_slots,
	       int nr)
{
	int result;
#ifdef DEBUG
	int i, j;
#endif

	result = ts->list->ops->read_mt(ts->list, samp, max_slots, nr);
#ifdef DEBUG
	for (j = 0; j < result; j++) {
		for (i = 0; i < max_slots; i++) {
			if (!(samp[j][i].valid & TSLIB_MT_VALID))
				continue;

			ts_error("TS_READ_MT----> slot %d: x = %d, y = %d, pressure = %d\n",
				 samp[j][i].slot, samp[j][i].x, samp[j][i].y,
				 samp[j][i].pressure);
		}
	}
#endif
	return result;

}
