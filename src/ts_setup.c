/*
 *  tslib/src/ts_setup.c
 *
 *  Copyright (C) 2017 Piotr Figlarek
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Find, open and configure a touchscreen device.
 */

#include "tslib.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static const char * const ts_name_default[] = {
		"/dev/input/ts",
		"/dev/input/touchscreen",
		"/dev/input/event0",
		"/dev/touchscreen/ucb1x00",
		NULL
};

struct tsdev *ts_setup(const char *dev_name, int nonblock)
{
	const char * const *defname;
	struct tsdev *ts = NULL;

	dev_name = dev_name ? dev_name : getenv("TSLIB_TSDEVICE");

	if (dev_name != NULL) {
		ts = ts_open(dev_name, nonblock);
	} else {
		defname = &ts_name_default[0];
		while (*defname != NULL) {
			ts = ts_open(*defname, nonblock);
			if (ts != NULL)
				break;

			++defname;
		}
	}

	/* if detected try to configure it */
	if (ts && ts_config(ts) != 0) {
		ts_error("ts_config: %s\n", strerror(errno));
		ts_close(ts);
		return NULL;
	}

	return ts;
}
