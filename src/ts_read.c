/*
 *  tslib/src/ts_read.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_read.c,v 1.3 2003/03/04 17:09:47 dlowder Exp $
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

/* This array is used to prevent segfaults and memory overwrites
 * that can occur if multiple events are returned from ts_read_raw
 * for each event returned by ts_read
 */
/* We found this was not needed, and have gone back to the
 * original implementation
 */

// static struct ts_sample ts_read_private_samples[1024];

int ts_read(struct tsdev *ts, struct ts_sample *samp, int nr)
{
	int result;
//	int i;
//	result = ts->list->ops->read(ts->list, ts_read_private_samples, nr);
	result = ts->list->ops->read(ts->list, samp, nr);
//	for(i=0;i<nr;i++) {
//		samp[i] = ts_read_private_samples[i];
//	}
#ifdef DEBUG
	fprintf(stderr,"TS_READ----> x = %d, y = %d, pressure = %d\n", samp->x, samp->y, samp->pressure);
#endif
	return result;

}
