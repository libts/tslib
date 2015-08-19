/*
 *  tslib/src/ts_open.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Open a touchscreen device.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/fcntl.h>

#include "tslib-private.h"

extern struct tslib_module_info __ts_raw;

int ts_close(struct tsdev *ts)
{
	if ( ts->fd >0) 
		return close( ts->fd);
	else
		return -1;
}
