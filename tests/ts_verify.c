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
	unsigned short verbose;

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
		data->samp_mt[i] = calloc(data->slots,
					  sizeof(struct ts_sample_mt));
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

/* TEST ts_read_mt 1 */
static int ts_verify_read_mt_1(struct ts_verify *data, int nr,
				     int nonblocking, int raw)
{
	int i, j;
	int ret;

	ret = ts_verify_alloc_mt(data, nr, nonblocking);
	if (ret < 0)
		return ret;

	if (raw == 0)
		ret = ts_read_mt(data->ts, data->samp_mt, data->slots, nr);
	else
		ret = ts_read_raw_mt(data->ts, data->samp_mt, data->slots, nr);
	if (ret == -EAGAIN) {
		ts_verify_free_mt(data);
		return ret;
	} else if (ret < 0) {
		perror("ts_read_mt");
		ts_verify_free_mt(data);
		return ret;
	}

	for (j = 0; j < ret; j++) {
		for (i = 0; i < data->slots; i++) {
			if (data->samp_mt[j][i].valid != 1)
				continue;

			if (data->samp_mt[j][i].pressure > 255) {
				ret = -1;
				if (data->verbose)
					printf("pressure out of bounds\n");
			}
			/* TODO check xy bounds when fbdev and not raw*/
		}
	}

	ts_verify_free_mt(data);

	return ret;
}

static int ts_verify_read_1(struct ts_verify *data, int nr,
			    int nonblocking, int raw)
{
	struct ts_sample samp[nr];
	int ret;

	data->ts = ts_setup(data->tsdevice, nonblocking);
	if (!data->ts) {
		perror("ts_setup");
		return errno;
	}

	if (raw == 0)
		ret = ts_read(data->ts, samp, nr);
	else
		ret = ts_read_raw(data->ts, samp, nr);
	if (ret < 0) {
		perror("ts_read");
	}

	return ret;
}

int main(int argc, char **argv)
{
	int ret;
	struct ts_verify data = {
		.ts = NULL,
		.tsdevice = NULL,
		.slots = 1,
		.samp_mt = NULL,
		.verbose = 0,
	};

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "idev",         required_argument, NULL, 'i' },
			{ "verbose",      no_argument,       NULL, 'v' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hi:v", long_options,
				    &option_index);

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

		case 'v':
			data.verbose = 1;
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

	/* data, samples, nonblocking, raw */
	ret = ts_verify_read_mt_1(&data, 1, 0, 0);
	if (ret == 1) {
		printf("TEST ts_read_mt (blocking) 1       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_mt (blocking) 1       ......   " RED "FAIL" RESET "\n");
	}

	ret = ts_verify_read_mt_1(&data, 5, 0, 0);
	if (ret == 5) {
		printf("TEST ts_read_mt (blocking) 5       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_mt (blocking) 5       ......   " RED "FAIL" RESET "\n");
	}

	while(1) {
		ret = ts_verify_read_mt_1(&data, 1, 1, 0);
		if (ret == -EAGAIN) {
			continue;
		} else if (ret == 1) {
			printf("TEST ts_read_mt (nonblocking) 1    ......   " GREEN "PASS" RESET "\n");
			break;
		} else {
			printf("TEST ts_read_mt (nonblocking) 1    ......   " RED "FAIL" RESET "\n");
			break;
		}
	}

	while(1) {
		ret = ts_verify_read_mt_1(&data, 5, 1, 0);
		if (ret == -EAGAIN) {
			continue;
		} else if (ret > 0 && ret <= 5) {
			printf("TEST ts_read_mt (nonblocking) 5    ......   " GREEN "PASS" RESET "\n");
			break;
		} else {
			printf("TEST ts_read_mt (nonblocking) 5    ......   " RED "FAIL" RESET "\n");
			break;
		}
	}

	ret = ts_verify_read_1(&data, 1, 0, 0);
	if (ret == 1) {
		printf("TEST ts_read    (blocking) 1       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read    (blocking) 1       ......   " RED "FAIL" RESET "\n");
	}

	ret = ts_verify_read_1(&data, 5, 0, 0);
	if (ret == 1) {
		printf("TEST ts_read    (blocking) 5       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read    (blocking) 5       ......   " RED "FAIL" RESET "\n");
	}



	ret = ts_verify_read_mt_1(&data, 1, 0, 1);
	if (ret == 1) {
		printf("TEST ts_read_raw_mt (blocking) 1   ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_raw_mt (blocking) 1   ......   " RED "FAIL" RESET "\n");
	}

	ret = ts_verify_read_mt_1(&data, 5, 0, 1);
	if (ret == 5) {
		printf("TEST ts_read_raw_mt (blocking) 5   ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_raw_mt (blocking) 5   ......   " RED "FAIL" RESET "\n");
	}

	while(1) {
		ret = ts_verify_read_mt_1(&data, 1, 1, 1);
		if (ret == -EAGAIN) {
			continue;
		} else if (ret == 1) {
			printf("TEST ts_read_raw_mt (nonblocking) 1......   " GREEN "PASS" RESET "\n");
			break;
		} else {
			printf("TEST ts_read_raw_mt (nonblocking) 1......   " RED "FAIL" RESET "\n");
			break;
		}
	}

	while(1) {
		ret = ts_verify_read_mt_1(&data, 5, 1, 1);
		if (ret == -EAGAIN) {
			continue;
		} else if (ret > 0 && ret <= 5) {
			printf("TEST ts_read_raw_mt (nonblocking) 5......   " GREEN "PASS" RESET "\n");
			break;
		} else {
			printf("TEST ts_read_raw_mt (nonblocking) 5......   " RED "FAIL" RESET "\n");
			break;
		}
	}

	ret = ts_verify_read_1(&data, 1, 0, 1);
	if (ret == 1) {
		printf("TEST ts_read_raw(blocking) 1       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_raw(blocking) 1       ......   " RED "FAIL" RESET "\n");
	}

	ret = ts_verify_read_1(&data, 5, 0, 1);
	if (ret == 1) {
		printf("TEST ts_read_raw(blocking) 5       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_raw(blocking) 5       ......   " RED "FAIL" RESET "\n");
	}

	return 0;
}
