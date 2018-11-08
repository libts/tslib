/*
 *  tslib/tests/ts_print_raw.c
 *
 *  Copyright (C) 2002 Douglas Lowder
 *  Just prints touchscreen events -- does not paint them on framebuffer
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 *
 * Basic test program for touchscreen library.
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>

#include "tslib.h"

static void usage(void)
{
	ts_print_ascii_logo(16);
	printf("%s", tslib_version());
	printf("\n");
	printf("-h --help\n");
	printf("                print this help text\n");
	printf("-v --version\n");
	printf("                print version information only\n");
}

int main(int argc, char **argv)
{
	struct tsdev *ts;

	while (1) {
		const struct option long_options[] = {
			{ "version",      no_argument,       NULL, 'v' },
			{ "help",         no_argument,       NULL, 'h' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "vh", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'v':
			printf("%s", tslib_version());
			return 0;

		case 'h':
			usage();
			return 0;

		default:
			usage();
			return 0;
		}
	}

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
			ts_close(ts);
			exit(1);
		}

		if (ret != 1)
			continue;

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec, samp.x, samp.y, samp.pressure);

	}

	ts_close(ts);
}
