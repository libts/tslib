/*
 *  tslib/src/ts_fd.c
 *
 *  Copyright (C) 2002 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_fd.c,v 1.1 2002/01/15 16:18:07 rmk Exp $
 *
 * Return the file descriptor for a touchscreen device.
 */
#include "tslib-private.h"

int ts_fd(struct tsdev *ts)
{
	return ts->fd;
}
