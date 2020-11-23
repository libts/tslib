/*
 *  tslib/tests/ts_verify.c
 *
 *  Copyright (C) 2017 Martin Kepplinger
 *
 * This file is part of tslib.
 *
 * ts_verify is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * ts_verify is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ts_verify.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 *
 * This program includes random tests of tslib's API
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
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

#include "tslib.h"
#include "testutils.h"

#ifndef ABS_MT_SLOT /* < 2.6.36 kernel headers */
# define ABS_MT_SLOT             0x2f    /* MT slot being modified */
#endif


#define CONFFILE "ts_verify_ts.conf"

struct ts_verify {
	struct tsdev *ts;
	char *tsdevice;
	uint8_t verbose;
	FILE *tsconf;

	struct ts_sample_mt **samp_mt;
	struct ts_sample *samp_read;
	int32_t slots;
	uint8_t nr;
	uint16_t read_mt_run_count;
	uint16_t read_mt_fail_count;
	uint16_t read_run_count;
	uint16_t read_fail_count;
	uint16_t iteration;
	uint16_t nr_of_iterations;
};

static void usage(char **argv)
{
	printf("%s\n", tslib_version());
	printf("Usage: %s [--idev <device>] [--verbose]\n", argv[0]);
}

static int ts_verify_alloc_mt(struct ts_verify *data, int nr, int8_t nonblocking)
{
#ifdef TS_HAVE_EVDEV
	struct input_absinfo slot;
#endif
	int i, j;

	data->ts = ts_setup(data->tsdevice, nonblocking);
	if (!data->ts) {
		perror("ts_setup");
		return errno;
	}

#ifdef TS_HAVE_EVDEV
	if (ioctl(ts_fd(data->ts), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
		perror("ioctl EVIOGABS");
		ts_close(data->ts);
		return errno;
	}

	data->slots = slot.maximum + 1 - slot.minimum;
#else
	data->slots = 11;
#endif

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

	for (i = 0; i < data->nr; i++)
		free(data->samp_mt[i]);
	free(data->samp_mt);
	data->samp_mt = NULL;

	ts_close(data->ts);
}

/* TEST ts_read_mt 1 */
static int ts_verify_read_mt_1(struct ts_verify *data, int nr,
				     int8_t nonblocking, int raw)
{
	int i, j;
	int ret;
	int count = 0;

	ret = ts_verify_alloc_mt(data, nr, nonblocking);
	if (ret < 0)
		return ret;

	/* blocking operation */
	if (nonblocking != 1) {
		if (raw == 0)
			ret = ts_read_mt(data->ts, data->samp_mt, data->slots, nr);
		else
			ret = ts_read_raw_mt(data->ts, data->samp_mt, data->slots, nr);
		if (ret < 0) {
			perror("ts_read_mt");
			ts_verify_free_mt(data);
			return ret;
		}

		for (j = 0; j < ret; j++) {
			for (i = 0; i < data->slots; i++) {
				if (!(data->samp_mt[j][i].valid & TSLIB_MT_VALID))
					continue;

				if (data->samp_mt[j][i].pressure > 255) {
					if (data->verbose)
						printf("pressure out of bounds\n");
				}
				/* TODO check xy bounds when fbdev and not raw*/
			}
		}
	/* non blocking operation */
	} else {
		while (1) {
			if (raw == 0)
				ret = ts_read_mt(data->ts, data->samp_mt, data->slots, nr);
			else
				ret = ts_read_raw_mt(data->ts, data->samp_mt, data->slots, nr);
			if (ret == -EAGAIN) {
				continue;
			} else if (ret < 0) {
				perror("ts_read_mt");
				ts_verify_free_mt(data);
				return ret;
			} else {
				count = count + ret;

				if (data->verbose)
					printf("got %d samples in 1 read\n", ret);
			}

			if (count >= nr) {
				ret = count;
				break;
			}
		}
	}

	ts_verify_free_mt(data);

	return ret;
}

static int ts_verify_read_1(struct ts_verify *data, int nr,
			    int nonblocking, int raw)
{
	int ret;

	if (!data->samp_read) {
		data->samp_read = calloc(nr, sizeof(struct ts_sample));
		if (!data->samp_read) {
			perror("calloc");
			return errno;
		}
	}

	data->ts = ts_setup(data->tsdevice, nonblocking);
	if (!data->ts) {
		free(data->samp_read);
		perror("ts_setup");
		return errno;
	}

	/* blocking */
	if (nonblocking != 1) {
		if (raw == 0)
			ret = ts_read(data->ts, data->samp_read, nr);
		else
			ret = ts_read_raw(data->ts, data->samp_read, nr);
	/*non blocking*/
	} else {
		while (1) {
			if (raw == 0)
				ret = ts_read(data->ts, data->samp_read, nr);
			else
				ret = ts_read_raw(data->ts, data->samp_read, nr);

	/* yeah that's lame but that's pretty much the only way it's used out there. it's deprecated anyways  */
			if (ret == nr)
				break;
		}
	}

	ts_close(data->ts);
	free(data->samp_read);
	data->samp_read = NULL;

	return ret;
}

static int ts_reconfig_1(struct ts_verify *data)
{
	int ret = 0;

	data->ts = ts_setup(data->tsdevice, 0);
	if (!data->ts) {
		perror("ts_setup");
		return errno;
	}

	ret = ts_reconfig(data->ts);
	if (data->verbose)
		printf("ts_reconfig ret: %d\n", ret);

	ts_close(data->ts);

	return ret;
}

static int ts_load_module_4_inv(struct ts_verify *data)
{
	int ret = 0;

	data->ts = ts_setup(data->tsdevice, 0);
	if (!data->ts) {
		perror("ts_setup");
		return errno;
	}

	ret = ts_load_module(data->ts, "asdf", "asdf");

	ts_close(data->ts);

	return ret;
}

static void run_tests(struct ts_verify *data)
{
	int32_t ret;

	printf("===================== test run %d =====================\n", data->iteration);

	/* ts_read() parameters(data, samples, nonblocking, raw) */
	ret = ts_verify_read_mt_1(data, 1, 0, 0);
	data->read_mt_run_count++;
	if (ret <= 1 && ret >= 0) {
		printf("TEST ts_read_mt (blocking) 1       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_mt (blocking) 1       ......   " RED "FAIL" RESET "\n");
		data->read_mt_fail_count++;
	}

	ret = ts_verify_read_mt_1(data, 5, 0, 0);
	data->read_mt_run_count++;
	if (ret <= 5 && ret > 1) { /* > 1 according to expected from filters */
		printf("TEST ts_read_mt (blocking) 5       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_mt (blocking) 5       ......   " RED "FAIL" RESET "\n");
		data->read_mt_fail_count++;
	}

	ret = ts_verify_read_mt_1(data, 1, 1, 0);
	data->read_mt_run_count++;
	if (ret <= 1 && ret >= 0) {
		printf("TEST ts_read_mt (nonblocking) 1    ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_mt (nonblocking) 1    ......   " RED "FAIL" RESET "\n");
		data->read_mt_fail_count++;
	}

	ret = ts_verify_read_mt_1(data, 5, 1, 0);
	data->read_mt_run_count++;
	if (ret <= 5 && ret > 1) { /* > 1 */
		printf("TEST ts_read_mt (nonblocking) 5    ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_mt (nonblocking) 5    ......   " RED "FAIL" RESET "\n");
		data->read_mt_fail_count++;
	}

	ret = ts_verify_read_1(data, 1, 0, 0);
	data->read_run_count++;
	if (ret <= 1 && ret >= 0) {
		printf("TEST ts_read    (blocking) 1       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read    (blocking) 1       ......   " RED "FAIL" RESET "\n");
		data->read_fail_count++;
	}

	ret = ts_verify_read_1(data, 5, 0, 0);
	data->read_run_count++;
	if (ret <= 5 && ret > 1) { /* > 1 */
		printf("TEST ts_read    (blocking) 5       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read    (blocking) 5       ......   " RED "FAIL" RESET "\n");
		data->read_fail_count++;
	}

	ret = ts_verify_read_1(data, 1, 1, 0);
	data->read_run_count++;
	if (ret <= 1 && ret >= 0) {
		printf("TEST ts_read    (nonblocking) 1    ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read    (nonblocking) 1    ......   " RED "FAIL" RESET "\n");
		data->read_fail_count++;
	}



	/* the same with the raw calls */
	ret = ts_verify_read_mt_1(data, 1, 0, 1);
	data->read_mt_run_count++;
	if (ret <= 1 && ret >= 0) {
		printf("TEST ts_read_raw_mt (blocking) 1   ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_raw_mt (blocking) 1   ......   " RED "FAIL" RESET "\n");
		data->read_mt_fail_count++;
	}

	ret = ts_verify_read_mt_1(data, 5, 0, 1);
	data->read_mt_run_count++;
	if (ret <= 5 && ret > 1) { /* > 1 */
		printf("TEST ts_read_raw_mt (blocking) 5   ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_raw_mt (blocking) 5   ......   " RED "FAIL" RESET "\n");
		data->read_mt_fail_count++;
	}

	ret = ts_verify_read_mt_1(data, 1, 1, 1);
	data->read_mt_run_count++;
	if (ret <= 1 && ret >= 0) {
		printf("TEST ts_read_raw_mt (nonblocking) 1......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_raw_mt (nonblocking) 1......   " RED "FAIL" RESET "\n");
		data->read_mt_fail_count++;
	}

	ret = ts_verify_read_mt_1(data, 5, 1, 1);
	data->read_mt_run_count++;
	if (ret <= 5 && ret > 1) { /* > 1 */
		printf("TEST ts_read_raw_mt (nonblocking) 5......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_raw_mt (nonblocking) 5......   " RED "FAIL" RESET "\n");
		data->read_mt_fail_count++;
	}

	ret = ts_verify_read_1(data, 1, 0, 1);
	data->read_run_count++;
	if (ret <= 1 && ret >= 0) {
		printf("TEST ts_read_raw(blocking) 1       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_raw(blocking) 1       ......   " RED "FAIL" RESET "\n");
		data->read_fail_count++;
	}

	ret = ts_verify_read_1(data, 5, 0, 1);
	data->read_run_count++;
	if (ret <= 5 && ret > 1) { /* > 1 */
		printf("TEST ts_read_raw(blocking) 5       ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_read_raw(blocking) 5       ......   " RED "FAIL" RESET "\n");
		data->read_fail_count++;
	}

	ret = ts_verify_read_1(data, 1, 1, 1);
	data->read_run_count++;
	if (ret <= 1 && ret >= 0) {
		printf("TEST ts_read_raw(nonblocking) 1    ......   " GREEN "PASS" RESET "\n");
	}



	ret = ts_reconfig_1(data);
	if (ret == 0) {
		printf("TEST ts_reconfig (1)               ......   " GREEN "PASS" RESET "\n");
	} else {
		printf("TEST ts_reconfig (1)               ......   " RED "FAIL" RESET "\n");
	}

	ret = ts_load_module_4_inv(data);
	if (ret == 0) {
		printf("TEST ts_load_module (4)            ......   " RED "FAIL" RESET "\n");
	} else {
		printf("TEST ts_load_module (4)            ......   " GREEN "PASS" RESET "\n");
	}

	data->iteration++;
}

int main(int argc, char **argv)
{
	struct ts_verify data = {
		.ts = NULL,
		.tsdevice = NULL,
		.slots = 1,
		.samp_mt = NULL,
		.samp_read = NULL,
		.verbose = 0,
		.read_fail_count = 0,
		.read_mt_fail_count = 0,
		.read_run_count = 0,
		.read_mt_run_count = 0,
		.iteration = 0,
		.nr_of_iterations = 6,
		.tsconf = NULL,
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
			return 0;
		}

		if (errno) {
			char str[9];
			sprintf(str, "option ?");
			str[7] = c & 0xff;
			perror(str);
		}
	}

	unlink(CONFFILE);
	data.tsconf = fopen(CONFFILE, "a+");
	fprintf(data.tsconf, "module_raw input\n");
	fclose(data.tsconf);

	if (setenv("TSLIB_CONFFILE", CONFFILE, 1) == -1)
		return errno;


	printf("%s\n", tslib_version());

	run_tests(&data);

	data.tsconf = fopen(CONFFILE, "a+");
	fprintf(data.tsconf, "module skip nhead=1 ntail=1\n");
	fclose(data.tsconf);
	printf("======================================================\n");
	printf("skip\n");

	run_tests(&data);

	data.tsconf = fopen(CONFFILE, "a+");
	fprintf(data.tsconf, "module pthres pmin=20\n");
	fclose(data.tsconf);
	printf("======================================================\n");
	printf("skip -> pthres\n");

	run_tests(&data);

	data.tsconf = fopen(CONFFILE, "a+");
	fprintf(data.tsconf, "module debounce drop_threshold=40\n");
	fclose(data.tsconf);
	printf("======================================================\n");
	printf("skip -> pthres -> debounce\n");

	run_tests(&data);

	data.tsconf = fopen(CONFFILE, "a+");
	fprintf(data.tsconf, "module median depth=7\n");
	fclose(data.tsconf);
	printf("======================================================\n");
	printf("skip -> pthres -> debounce -> median\n");

	run_tests(&data);

	data.tsconf = fopen(CONFFILE, "a+");
	fprintf(data.tsconf, "module dejitter delta=100\n");
	fclose(data.tsconf);
	printf("======================================================\n");
	printf("skip -> pthres -> debounce -> median -> dejitter\n");

	run_tests(&data);

	data.tsconf = fopen(CONFFILE, "a+");
	fprintf(data.tsconf, "module lowpass\n");
	fclose(data.tsconf);
	printf("======================================================\n");
	printf("skip -> pthres -> debounce -> median -> dejitter -> lowpass\n");

	run_tests(&data);

	data.tsconf = fopen(CONFFILE, "a+");
	fprintf(data.tsconf, "module invert\n");
	fclose(data.tsconf);
	printf("======================================================\n");
	printf("skip -> pthres -> debounce -> median -> dejitter -> lowpass -> invert\n");

	run_tests(&data);

	data.tsconf = fopen(CONFFILE, "a+");
	fprintf(data.tsconf, "module linear\n");
	fclose(data.tsconf);
	printf("======================================================\n");
	printf("skip -> pthres -> debounce -> median -> dejitter -> lowpass -> invert -> linear\n");

	run_tests(&data);

	unlink(CONFFILE);

	printf("------------------------------------------------------\n");
	printf("summary of results:\n");
	printf("------------------------------------------------------\n");
	printf("read_mt FAILs: %3d of %3d\n", data.read_mt_fail_count, data.read_mt_run_count);
	printf("read    FAILs: %3d of %3d (WONTFIX)\n", data.read_fail_count, data.read_run_count);
	printf("------------------------------------------------------\n");

	return 0;
}
