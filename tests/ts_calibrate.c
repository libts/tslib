/*
 *  tslib/tests/ts_calibrate.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 *
 * Graphical touchscreen calibration tool. This writes the configuration
 * file used by tslib's "linear" filter plugin module to transform the
 * touch samples according to the calibration.
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>

#include "tslib.h"

#include "fbutils.h"
#include "testutils.h"
#include "ts_calibrate.h"

#define CROSS_BOUND_DIST	50
#define VALIDATE_BOUNDARY_MIN	10
#define VALIDATE_LOOPS_DEFAULT	3

static int palette[] = {
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0xff0000, 0x00ff00
};
#define NR_COLORS (sizeof(palette) / sizeof(palette[0]))

static void sig(int sig)
{
	close_framebuffer();
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
}

static unsigned int getticks()
{
	static struct timeval ticks = {0};
	static unsigned int val = 0;

	gettimeofday(&ticks, NULL);
	val = ticks.tv_sec * 1000;
	val += ticks.tv_usec / 1000;

	return val;
}

static void get_sample(struct tsdev *ts, calibration *cal,
		       int index, int x, int y, char *name, short redo)
{
	static int last_x = -1, last_y;

	if (redo) {
		last_x = -1;
		last_y = 0;
	}

	if (last_x != -1) {
#define NR_STEPS 10
		int dx = ((x - last_x) << 16) / NR_STEPS;
		int dy = ((y - last_y) << 16) / NR_STEPS;
		int i;

		last_x <<= 16;
		last_y <<= 16;
		for (i = 0; i < NR_STEPS; i++) {
			put_cross(last_x >> 16, last_y >> 16, 2 | XORMODE);
			usleep(1000);
			put_cross(last_x >> 16, last_y >> 16, 2 | XORMODE);
			last_x += dx;
			last_y += dy;
		}
	}

	put_cross(x, y, 2 | XORMODE);
	getxy(ts, &cal->x[index], &cal->y[index]);
	put_cross(x, y, 2 | XORMODE);

	last_x = cal->xfb[index] = x;
	last_y = cal->yfb[index] = y;

	printf("%s : X = %4d Y = %4d\n", name, cal->x[index], cal->y[index]);
}

static int validate_sample(struct tsdev *ts, int x, int y, char *name,
			   int boundary)
{
	static int last_x = -1, last_y;
	int read_x, read_y;
	int ret;

	if (last_x != -1) {
#define NR_STEPS 10
		int dx = ((x - last_x) << 16) / NR_STEPS;
		int dy = ((y - last_y) << 16) / NR_STEPS;
		int i;

		last_x <<= 16;
		last_y <<= 16;
		for (i = 0; i < NR_STEPS; i++) {
			put_cross(last_x >> 16, last_y >> 16, 2 | XORMODE);
			usleep(1000);
			put_cross(last_x >> 16, last_y >> 16, 2 | XORMODE);
			last_x += dx;
			last_y += dy;
		}
	}

	put_cross(x, y, 2 | XORMODE);
	getxy_validate(ts, &read_x, &read_y);

	if ((read_x > x - boundary) && (read_x < x + boundary) &&
	    (read_y > y - boundary) && (read_y < y + boundary)) {
		ret = 0;
	} else {
		ret = -1;
	}
	put_cross(x, y, 2 | XORMODE);

	last_x = x;
	last_y = y;

	printf("%s : X = %4d(%4d) Y = %4d(%4d) %s\n",
	       name, read_x, x, read_y, y,
	       ret ? "fail" : "pass");

	return ret;
}

static int ts_validate(struct tsdev *ts, int boundary, unsigned int loops, int timeout)
{
	int ret;
	char textbuf[64];
	int random_x, random_y;
	unsigned int i;

	if (boundary < VALIDATE_BOUNDARY_MIN ||
	    boundary > (int)xres || boundary > (int)yres) {
		boundary = VALIDATE_BOUNDARY_MIN;
		fprintf(stderr, "Boundary out of range. Using %d\n",
			boundary);
	}

	if (loops == 0)
		loops = VALIDATE_LOOPS_DEFAULT;

	snprintf(textbuf, sizeof(textbuf),
		 "Validate touchscreen calibration with boundary %d.",
		 boundary);

	put_string_center(xres / 2, yres / 4, textbuf, 1);
	put_string_center(xres / 2, yres / 4 + 20,
			  "Touch crosshair to validate", 2);

	for (i = 0; i < loops; i++) {
		srand(time(NULL));
		random_x = rand() % xres;
		random_y = rand() % yres;
		ret = validate_sample(ts, random_x, random_y, "random", boundary);
		if (ret)
			goto done;
	}

done:
	fillrect(0, 0, xres - 1, yres - 1, 0);
	put_string_center(xres / 2, yres / 4, textbuf, 1);

	if (timeout == 0) {
		close_framebuffer();
		ts_close(ts);
		return ret;
	}

	if (!ret) {
		printf("Validation passed.\n");
		put_string_center(xres / 2, yres / 4 + 20,
				  "Validation passed", 5);
	} else {
		printf("Validation failed.\n");
		put_string_center(xres / 2, yres / 4 + 20,
				  "Validation failed", 4);
	}

	if (timeout > 0) {
		sleep(timeout);
		close_framebuffer();
		ts_close(ts);
	}

	return ret;
}

static void clearbuf(struct tsdev *ts)
{
	int fd = ts_fd(ts);
	fd_set fdset;
	struct timeval tv;
	int nfds;
	struct ts_sample sample;

	while (1) {
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);

		tv.tv_sec = 0;
		tv.tv_usec = 0;

		nfds = select(fd + 1, &fdset, NULL, NULL, &tv);
		if (nfds == 0)
			break;

		if (ts_read_raw(ts, &sample, 1) < 0) {
			perror("ts_read_raw");
			exit(1);
		}
	}
}

static void help(void)
{
	ts_print_ascii_logo(16);
	print_version();

	printf("\n");
	printf("Usage: ts_calibrate [-r <rotate_value>] [--version]\n");
	printf("\n");
	printf("-r --rotate\n");
	printf("        <rotate_value> 0 ... no rotation; 0 degree (default)\n");
	printf("                       1 ... clockwise orientation; 90 degrees\n");
	printf("                       2 ... upside down orientation; 180 degrees\n");
	printf("                       3 ... counterclockwise orientation; 270 degrees\n");
	printf("-t --min_interval\n");
	printf("                       minimum time in ms between touch presses\n");
	printf("-c --validate\n");
	printf("                       validate the current calibration\n");
	printf("-b --boundary\n");
	printf("                       boundary criteria in validation mode\n");
	printf("-l --loops\n");
	printf("                       number of crosses to touch in validation mode\n");
	printf("-s --timeout\n");
	printf("                       result screen timeout in seconds in validation mode\n");
	printf("                       -1 ... no timeout\n");
	printf("                        0 ... no result screen (quit immediately)\n");
	printf("                        5 ... 5s timeout for result screen (default)\n");
	printf("-h --help\n");
	printf("                       print this help text\n");
	printf("-v --version\n");
	printf("                       print version information only\n");
	printf("\n");
	printf("Example (Linux): ts_calibrate -r $(cat /sys/class/graphics/fbcon/rotate)\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	struct tsdev *ts;
	calibration cal = {
		.x = { 0 },
		.y = { 0 },
	};
	int cal_fd;
	char cal_buffer[256];
	char *calfile = NULL;
	unsigned int i, len;
	unsigned int tick = 0;
	unsigned int min_interval = 0;
	int boundary = VALIDATE_BOUNDARY_MIN;
	int validate_timeout = 5;
	unsigned int validate_loops = 0;
	short validate_only = 0;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "rotate",       required_argument, NULL, 'r' },
			{ "version",      no_argument,       NULL, 'v' },
			{ "min_interval", required_argument, NULL, 't' },
			{ "validate",     no_argument,       NULL, 'c' },
			{ "boundary",     required_argument, NULL, 'b' },
			{ "loop",         required_argument, NULL, 'l' },
			{ "timeout",      required_argument, NULL, 's' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hvr:t:cb:l:s:", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			help();
			return 0;

		case 'v':
			print_version();
			return 0;

		case 'r':
			/* extern in fbutils.h */
			rotation = atoi(optarg);
			if (rotation < 0 || rotation > 3) {
				help();
				return 0;
			}
			break;

		case 't':
			min_interval = atoi(optarg);
			if (min_interval > 10000) {
				fprintf(stderr, "Minimum interval too long\n");
				return 0;
			}
			break;

		case 'c':
			validate_only = 1;
			break;

		case 'b':
			if (!validate_only) {
				fprintf(stderr, "--boundary is only available with --validate\n");
				help();
				return 0;
			}

			boundary = atoi(optarg);
			break;

		case 'l':
			if (!validate_only) {
				fprintf(stderr, "--loop is only available with --validate\n");
				help();
				return 0;
			}

			validate_loops = atoi(optarg);
			break;

		case 's':
			if (!validate_only) {
				fprintf(stderr, "--timeout is only available with --validate\n");
				help();
				return 0;
			}

			validate_timeout = atoi(optarg);
			break;

		default:
			help();
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

	if (open_framebuffer()) {
		close_framebuffer();
		ts_close(ts);
		exit(1);
	}

	for (i = 0; i < NR_COLORS; i++)
		setcolor(i, palette[i]);

	if (validate_only)
		return ts_validate(ts, boundary, validate_loops, validate_timeout);

	put_string_center(xres / 2, yres / 4,
			  "Touchscreen calibration utility", 1);
	put_string_center(xres / 2, yres / 4 + 20,
			  "Touch crosshair to calibrate", 2);

	printf("xres = %d, yres = %d\n", xres, yres);

	/* Clear the buffer */
	clearbuf(ts);

	/* ignore rotation for calibration. only save it.*/
	int rotation_temp = rotation;
	int xres_temp = xres;
	int yres_temp = yres;
	rotation = 0;
	xres = xres_orig;
	yres = yres_orig;

	short redo = 0;

redocalibration:
	tick = getticks();
	get_sample(ts, &cal, 0, CROSS_BOUND_DIST,        CROSS_BOUND_DIST,        "Top left", redo);
	redo = 0;
	if (getticks() - tick < min_interval) {
		redo = 1;
	#ifdef DEBUG
		printf("ts_calibrate: time before touch press < %dms. restarting.\n",
			min_interval);
	#endif
		goto redocalibration;
	}
	clearbuf(ts);

	tick = getticks();
	get_sample(ts, &cal, 1, xres - CROSS_BOUND_DIST, CROSS_BOUND_DIST,        "Top right", redo);
	if (getticks() - tick < min_interval) {
		redo = 1;
	#ifdef DEBUG
		printf("ts_calibrate: time before touch press < %dms. restarting.\n",
			min_interval);
	#endif
		goto redocalibration;
	}
	clearbuf(ts);

	tick = getticks();
	get_sample(ts, &cal, 2, xres - CROSS_BOUND_DIST, yres - CROSS_BOUND_DIST, "Bot right", redo);
	if (getticks() - tick < min_interval) {
		redo = 1;
	#ifdef DEBUG
		printf("ts_calibrate: time before touch press < %dms. restarting.\n",
			min_interval);
	#endif
		goto redocalibration;
	}
	clearbuf(ts);

	tick = getticks();
	get_sample(ts, &cal, 3, CROSS_BOUND_DIST,        yres - CROSS_BOUND_DIST, "Bot left", redo);
	if (getticks() - tick < min_interval) {
		redo = 1;
	#ifdef DEBUG
		printf("ts_calibrate: time before touch press < %dms. restarting.\n",
			min_interval);
	#endif
		goto redocalibration;
	}
	clearbuf(ts);

	tick = getticks();
	get_sample(ts, &cal, 4, xres_orig / 2,  yres_orig / 2,  "Center", redo);
	if (getticks() - tick < min_interval) {
		redo = 1;
	#ifdef DEBUG
		printf("ts_calibrate: time before touch press < %dms. restarting.\n",
			min_interval);
	#endif
		goto redocalibration;
	}

	rotation = rotation_temp;
	xres = xres_temp;
	yres = yres_temp;

	if (perform_calibration (&cal)) {
		printf("Calibration constants: ");
		for (i = 0; i < 7; i++)
			printf("%d ", cal.a[i]);
		printf("\n");
		if ((calfile = getenv("TSLIB_CALIBFILE")) != NULL) {
			cal_fd = open(calfile, O_CREAT | O_TRUNC | O_RDWR,
				      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		} else {
			cal_fd = open(TS_POINTERCAL, O_CREAT | O_TRUNC | O_RDWR,
				      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		}
		if (cal_fd < 0) {
			perror("open");
			close_framebuffer();
			ts_close(ts);
			exit(1);
		}

		len = sprintf(cal_buffer, "%d %d %d %d %d %d %d %d %d %d",
			      cal.a[1], cal.a[2], cal.a[0],
			      cal.a[4], cal.a[5], cal.a[3], cal.a[6],
			      xres_orig, yres_orig, rotation);
		if (write(cal_fd, cal_buffer, len) == -1) {
			perror("write");
			close_framebuffer();
			ts_close(ts);
			exit(1);
		}
		close(cal_fd);
		i = 0;
	} else {
		printf("Calibration failed.\n");
		i = -1;
	}

	fillrect(0, 0, xres - 1, yres - 1, 0);
	close_framebuffer();
	ts_close(ts);
	return i;
}
