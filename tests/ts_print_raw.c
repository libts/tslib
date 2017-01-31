/*
 *  tslib/tests/ts_print_raw.c
 *
 *  Derived from tslib/src/ts_test.c by Douglas Lowder
 *  Just prints touchscreen events -- does not paint them on framebuffer
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * Basic test program for touchscreen library.
 */
#include <stdio.h>
#include <stdlib.h>

#include "tslib.h"


int main()
{
	struct tsdev *ts;

	ts = ts_setup(NULL, 0);
	if (!ts) {
		perror("ts_setup");
		exit(1);
	}

	while (1) {
		struct ts_sample samp;
		int ret;

		ret = ts_read_raw(ts, &samp, 1);

		if (ret < 0) {
			perror("ts_read_raw");
			exit(1);
		}

		if (ret != 1)
			continue;

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec, samp.x, samp.y, samp.pressure);

	}
}
