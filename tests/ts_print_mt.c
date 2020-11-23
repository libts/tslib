/*
 *  tslib/tests/ts_print_mt.c
 *
 *  Copyright (C) 2017 Martin Kepplinger
 *
 * This file is part of tslib.
 *
 * ts_print_mt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * ts_print_mt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ts_print_mt.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 *
 * Just prints touchscreen events -- does not paint them on framebuffer
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>

#if defined (__FreeBSD__)

#include <dev/evdev/input.h>
#define TS_HAVE_EVDEV

#elif defined (__linux__)

#include <linux/input.h>
#define TS_HAVE_EVDEV

#endif

#ifdef TS_HAVE_EVDEV
#include <sys/ioctl.h>
#endif

#include "tslib.h"
#include "testutils.h"

#ifndef ABS_MT_SLOT /* < 2.6.36 kernel headers */
# define ABS_MT_SLOT             0x2f    /* MT slot being modified */
#endif

static void usage(char **argv)
{
	ts_print_ascii_logo(16);
	printf("%s", tslib_version());
	printf("\n");
	printf("Usage: %s [--raw] [--non-blocking] [-s <samples>] [-i <device>]\n",
		argv[0]);
	printf("\n");
	printf("-r --raw\n");
	printf("                don't apply filter modules. Use what module_raw\n");
	printf("                delivers directly.\n");
	printf("-n --non-blocking\n");
	printf("                tests non-blocking read and loops\n");
	printf("-i --idev\n");
	printf("                explicitly choose the touch input device\n");
	printf("                overriding TSLIB_TSDEVICE\n");
	printf("-s --samples\n");
	printf("                number of samples to request ts_read_mt() to\n");
	printf("		get before proceeding\n");
	printf("-j --slots\n");
	printf("                set the number of concurrently available touch\n");
	printf("                points. This overrides multitouch slots for\n");
	printf("                testing purposes.\n");
	printf("-h --help\n");
	printf("                print this help text\n");
	printf("-v --version\n");
	printf("                print version information only\n");
}

static int errfn(const char *fmt, va_list ap)
{
	return vfprintf(stderr, fmt, ap);
}

static int openfn(const char *path, int flags,
		  void *user_data __attribute__((unused)))
{
	return open(path, flags);
}

int main(int argc, char **argv)
{
	struct tsdev *ts;
	char *tsdevice = NULL;
	struct ts_sample_mt **samp_mt = NULL;
#ifdef TS_HAVE_EVDEV
	struct input_absinfo slot;
#endif
	int32_t user_slots = 0;
	int32_t max_slots = 1;
	int ret, i, j;
	int read_samples = 1;
	short non_blocking = 0;
	short raw = 0;
	struct ts_lib_version_data *ver = ts_libversion();

#ifndef TSLIB_VERSION_MT /* < 1.10 */
	printf("You are running an old version of tslib. Please upgrade.\n");
#endif

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "idev",         required_argument, NULL, 'i' },
			{ "samples",      required_argument, NULL, 's' },
			{ "non-blocking", no_argument,       NULL, 'n' },
			{ "raw",          no_argument,       NULL, 'r' },
			{ "slots",        required_argument, NULL, 'j' },
			{ "version",      no_argument,       NULL, 'v' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hvi:s:nrj:", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage(argv);
			return 0;

		case 'v':
			printf("%s\n", tslib_version());
			return 0;

		case 'i':
			tsdevice = optarg;
			break;

		case 'n':
			non_blocking = 1;
			break;

		case 'r':
			raw = 1;
			break;

		case 's':
			read_samples = atoi(optarg);
			if (read_samples <= 0) {
				usage(argv);
				return 0;
			}
			break;

		case 'j':
			user_slots = atoi(optarg);
			if (user_slots <= 0) {
				usage(argv);
				return 0;
			}
			break;

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

	ts_error_fn = errfn;

#ifdef TSLIB_VERSION_OPEN_RESTRICTED
	if (ver->features & TSLIB_VERSION_OPEN_RESTRICTED)
		ts_open_restricted = openfn;
#endif

	if (non_blocking)
		ts = ts_setup(tsdevice, 1);
	else
		ts = ts_setup(tsdevice, 0);

	if (!ts) {
		perror("ts_setup");
		return errno;
	}

	printf("libts %06X opened device %s\n",
	       ver->version_num, ts_get_eventpath(ts));

#ifdef TS_HAVE_EVDEV
	if (ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
		perror("ioctl EVIOGABS");
		ts_close(ts);
		return errno;
	}

	max_slots = slot.maximum + 1 - slot.minimum;
#endif
	if (user_slots > 0)
		max_slots = user_slots;

	samp_mt = malloc(read_samples * sizeof(struct ts_sample_mt *));
	if (!samp_mt) {
		ts_close(ts);
		return -ENOMEM;
	}
	for (i = 0; i < read_samples; i++) {
		samp_mt[i] = calloc(max_slots, sizeof(struct ts_sample_mt));
		if (!samp_mt[i]) {
			for (i--; i >= 0; i--)
				free(samp_mt[i]);
			free(samp_mt);
			ts_close(ts);
			return -ENOMEM;
		}
	}

	while (1) {
		if (raw)
			ret = ts_read_raw_mt(ts, samp_mt, max_slots, read_samples);
		else
			ret = ts_read_mt(ts, samp_mt, max_slots, read_samples);

		if (ret < 0) {
			if (non_blocking) {
				printf("ts_print_mt: read returns %d\n", ret);
				continue;
			}

			perror("ts_read_mt");
			ts_close(ts);
			exit(1);
		}

		for (j = 0; j < ret; j++) {
			for (i = 0; i < max_slots; i++) {
				if (!(samp_mt[j][i].valid & TSLIB_MT_VALID))
					continue;

				printf(YELLOW "sample %d - %ld.%06ld -" RESET " (slot %d) %6d %6d %6d\n",
				       j,
				       samp_mt[j][i].tv.tv_sec,
				       samp_mt[j][i].tv.tv_usec,
				       samp_mt[j][i].slot,
				       samp_mt[j][i].x,
				       samp_mt[j][i].y,
				       samp_mt[j][i].pressure);
			}
		}
	}

	ts_close(ts);
}
