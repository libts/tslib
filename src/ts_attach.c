/*
 *  tslib/src/ts_attach.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_attach.c,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Attach a filter to a touchscreen device.
 */
#include "config.h"

#include "tslib-private.h"

int __ts_attach(struct tsdev *ts, struct tslib_module_info *info)
{
	info->dev = ts;
	info->next = ts->list;
	ts->list = info;

	return 0;
}
