/*
 *  tslib/src/ts_get_eventpath.c
 *
 *  Copyright (C) 2017 Martin Kepplinger
 *
 * SPDX-License-Identifier: LGPL-2.1
 */
#include "tslib-private.h"

char *ts_get_eventpath(struct tsdev *tsdev)
{
	return tsdev->eventpath;
}
