/*
 *  tslib/src/ts_read.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_read.c,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Read raw pressure, x, y, and timestamp from a touchscreen device.
 */
#include "config.h"

#include "tslib-private.h"

int ts_read(struct tsdev *ts, struct ts_sample *samp, int nr)
{
	return ts->list->ops->read(ts->list, samp, nr);
}
