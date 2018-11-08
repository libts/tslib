/*
 *  tslib/tests/ts_print.c
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

static void usage(char **argv)
{
	ts_print_ascii_logo(16);
	printf("%s", tslib_version());
	printf("\n");
	printf("Usage: %s [--raw]\n", argv[0]);
	printf("\n");
	printf("-r --raw\n");
	printf("                don't apply filter modules. use what module_raw\n");
	printf("                delivers directly. This is equivalent to\n");
	printf("                running the ts_print_raw program\n");
	printf("-h --help\n");
	printf("                print this help text\n");
	printf("-v --version\n");
	printf("                print version information only\n");
}

int main(int argc, char **argv)
{
	struct tsdev *ts;
	short raw = 0;

	while (1) {
		const struct option long_options[] = {
			{ "version",      no_argument,       NULL, 'v' },
			{ "help",         no_argument,       NULL, 'h' },
			{ "raw",          no_argument,       NULL, 'r' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "vrh", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'r':
			raw = 1;
			break;

		case 'v':
			printf("%s", tslib_version());
			return 0;

		case 'h':
			usage(argv);
			return 0;

		default:
			usage(argv);
			return 0;
		}

		if (errno) {
			char str[9];

			sprintf(str, "option ?");
			str[7] = c & 0xff;
			perror(str);
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

		if (raw)
			ret = ts_read_raw(ts, &samp, 1);
		else
			ret = ts_read(ts, &samp, 1);

		if (ret < 0) {
			perror("ts_read");
			ts_close(ts);
			exit(1);
		}

		if (ret != 1)
			continue;

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec, samp.x, samp.y, samp.pressure);

	}

	ts_close(ts);
}
