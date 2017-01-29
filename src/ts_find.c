/*
 *  tslib/src/ts_find.c
 *
 *  Copyright (C) 2017 Piotr Figlarek
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Find and open a touchscreen device.
 */

#include "tslib.h"
#include <stdlib.h>

static const char * const ts_name_default[] = {
		"/dev/input/ts",
		"/dev/input/touchscreen",
		"/dev/input/event0",
		"/dev/touchscreen/ucb1x00",
		NULL
};

struct tsdev *ts_find(int nonblock)
{
	struct tsdev *ts = NULL;
	const char *name = getenv("TSLIB_TSDEVICE");

	if (name != NULL) {
		ts = ts_open(name, nonblock);
	} else {
		for (name = ts_name_default[0]; name != NULL; name++) {
			ts = ts_open(name, nonblock);
			if (ts != NULL)
				break;
		}
	}

	return ts;
}
