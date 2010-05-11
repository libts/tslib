/*
 *  tslib/src/ts_fd.c
 *
 *  Copyright (C) 2002 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Return the file descriptor for a touchscreen device.
 */
#include "tslib-private.h"

int ts_fd(struct tsdev *ts)
{
	return ts->fd;
}
