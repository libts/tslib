/*
 *  tslib/tests/ts_print_mt.c
 *
 * Just prints touchscreen events -- does not paint them on framebuffer
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>

#ifdef __FreeBSD__
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

#include "tslib.h"
#include "testutils.h"

int main(int argc, char **argv)
{
	struct tsdev *ts;
	char *tsdevice = NULL;
	struct ts_sample_mt **samp_mt = NULL;
	struct input_absinfo slot;
	unsigned short max_slots = 1;
	int ret, i;

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       0, 'h' },
			{ "idev",         required_argument, 0, 'i' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hi:", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			printf("Usage: %s [-i <device>]\n", argv[0]);
			return 0;

		case 'i':
			tsdevice = optarg;
			break;

		default:
			printf("Usage: %s [-i <device>]\n", argv[0]);
			break;
		}

		if (errno) {
			char *str = "option ?";
			str[7] = c & 0xff;
			perror(str);
		}
	}

	ts = ts_setup(tsdevice, 0);
	if (!ts) {
		perror("ts_setup");
		return errno;
	}

	if (ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
		perror("ioctl EVIOGABS");
		ts_close(ts);
		return errno;
	}

	max_slots = slot.maximum + 1 - slot.minimum;

	samp_mt = malloc(sizeof(struct ts_sample_mt *));
	if (!samp_mt) {
		ts_close(ts);
		return -ENOMEM;
	}
	samp_mt[0] = calloc(max_slots, sizeof(struct ts_sample_mt));
	if (!samp_mt[0]) {
		free(samp_mt);
		ts_close(ts);
		return -ENOMEM;
	}

	while (1) {
		ret = ts_read_mt(ts, samp_mt, max_slots, 1);
		if (ret < 0) {
			perror("ts_read_raw_mt");
			ts_close(ts);
			exit(1);
		}

		if (ret != 1)
			continue;

		for (i = 0; i < max_slots; i++) {
			if (samp_mt[0][i].valid != 1)
				continue;

			printf(YELLOW "%ld.%06ld:" RESET " (slot %d) %6d %6d %6d\n",
			       samp_mt[0][i].tv.tv_sec,
			       samp_mt[0][i].tv.tv_usec,
			       samp_mt[0][i].slot,
			       samp_mt[0][i].x,
			       samp_mt[0][i].y,
			       samp_mt[0][i].pressure);
		}
	}

	ts_close(ts);
}
