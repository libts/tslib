/*
 *  tslib/tests/ts_print_mt.c
 *
 *  Copyright (C) 2016 Martin Kepplinger
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

struct ts_verify {
	struct tsdev *ts;
	char *tsdevice;

	struct ts_sample_mt **samp_mt;
	unsigned short slots;
	unsigned short nr;
};

static void usage(char **argv)
{
	printf("tslib " PACKAGE_VERSION "\n");
	printf("\n");
	printf("Usage: %s [-i <device>]\n", argv[0]);
}

static int ts_verify_alloc_mt(struct ts_verify *data, int nr, short nonblocking)
{
	struct input_absinfo slot;
	int i, j;

	data->ts = ts_setup(data->tsdevice, nonblocking);
	if (!data->ts) {
		perror("ts_setup");
		return errno;
	}

	if (ioctl(ts_fd(data->ts), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
		perror("ioctl EVIOGABS");
		ts_close(data->ts);
		return errno;
	}

	data->slots = slot.maximum + 1 - slot.minimum;

	data->samp_mt = malloc(nr * sizeof(struct ts_sample_mt *));
	if (!data->samp_mt) {
		ts_close(data->ts);
		return -ENOMEM;
	}
	for (i = 0; i < nr; i++) {
		data->samp_mt[i] = calloc(data->slots, sizeof(struct ts_sample_mt));
		if (!data->samp_mt[i]) {
			for (j = 0; j < i; j++)
				free(data->samp_mt[j]);

			free(data->samp_mt);
			ts_close(data->ts);
			return -ENOMEM;
		}
	}

	return 0;
}

static void ts_verify_free_mt(struct ts_verify *data)
{
	int i;

	if (!data->samp_mt)
		return;

	for (i = 0; i < data->nr; i++) {
		if (data->samp_mt[i])
			free(data->samp_mt[i]);
	}
	free(data->samp_mt);
	ts_close(data->ts);
}

/* TEST ts_read_mt - blocking 1 */
static int ts_verify_read_blocking_1(struct ts_verify *data, int nr)
{
	int i, j;
	int ret;
	short valid_found = 0;

	/* allocate NOT nonblocking */
	ret = ts_verify_alloc_mt(data, nr, 0);
	if (ret < 0)
		return ret;

	ret = ts_read_mt(data->ts, data->samp_mt, data->slots, nr);
	if (ret < 0) {
		perror("ts_read_mt");
		ts_close(data->ts);
		return ret;
	}

	for (j = 0; j < ret; j++) {
		for (i = 0; i < data->slots; i++) {
			if (data->samp_mt[j][i].valid != 1)
				continue;

			valid_found = 1;
		}
	}

	if (valid_found == 1)
		return ret;
	else
		return -1;
}

int main(int argc, char **argv)
{
	int ret;
	struct ts_verify data = {
		.ts = NULL,
		.tsdevice = NULL,
		.slots = 1,
		.samp_mt = NULL,
	};

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "idev",         required_argument, NULL, 'i' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hi:s:nr", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage(argv);
			return 0;

		case 'i':
			data.tsdevice = optarg;
			break;

		default:
			usage(argv);
			break;
		}

		if (errno) {
			char *str = "option ?";
			str[7] = c & 0xff;
			perror(str);
		}
	}

	ret = ts_verify_read_blocking_1(&data, 1);
	if (ret == 1) {
		printf("TEST ts_read_mt (blocking) 1     ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_mt (blocking) 1     ......   " RED "FAIL" RESET "\n");
	}

	ret = ts_verify_read_blocking_1(&data, 5);
	if (ret == 5) {
		printf("TEST ts_read_mt (blocking) 5     ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_mt (blocking) 5     ......   " RED "FAIL" RESET "\n");
	}

	return 0;
}
