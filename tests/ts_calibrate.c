/*
 *  tslib/tests/ts_calibrate.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Basic test program for touchscreen library.
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>

#include "tslib.h"

#include "fbutils.h"
#include "testutils.h"
#include "ts_calibrate.h"

static int palette[] = {
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0
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

static void get_sample(struct tsdev *ts, calibration *cal,
		       int index, int x, int y, char *name)
{
	static int last_x = -1, last_y;

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
			perror("ts_read");
			exit(1);
		}
	}
}

static void help(void)
{
	struct ts_lib_version_data *ver = ts_libversion();

	printf("tslib %s (library 0x%X)\n", ver->package_version, ver->version_num);
	printf("\n");
	printf("Usage: ts_calibrate [-r <rotate_value>]\n");
	printf("\n");
	printf("        <rotate_value> 0 ... no rotation; 0 degree (default)\n");
	printf("                       1 ... clockwise orientation; 90 degrees\n");
	printf("                       2 ... upside down orientation; 180 degrees\n");
	printf("                       3 ... counterclockwise orientation; 270 degrees\n");
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

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "rotate",       required_argument, NULL, 'r' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hr:", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			help();
			return 0;

		case 'r':
			/* extern in fbutils.h */
			rotation = atoi(optarg);
			if (rotation < 0 || rotation > 3) {
				help();
				return 0;
			}
			break;

		default:
			help();
			return 0;
		}

		if (errno) {
			char *str = "option ?";
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

	put_string_center(xres / 2, yres / 4,
			  "Touchscreen calibration utility", 1);
	put_string_center(xres / 2, yres / 4 + 20,
			  "Touch crosshair to calibrate", 2);

	printf("xres = %d, yres = %d\n", xres, yres);

	/* Clear the buffer */
	clearbuf(ts);

	get_sample(ts, &cal, 0, 50,        50,        "Top left");
	clearbuf(ts);
	get_sample(ts, &cal, 1, xres - 50, 50,        "Top right");
	clearbuf(ts);
	get_sample(ts, &cal, 2, xres - 50, yres - 50, "Bot right");
	clearbuf(ts);
	get_sample(ts, &cal, 3, 50,        yres - 50, "Bot left");
	clearbuf(ts);
	get_sample(ts, &cal, 4, xres / 2,  yres / 2,  "Center");

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

		len = sprintf(cal_buffer, "%d %d %d %d %d %d %d %d %d",
			      cal.a[1], cal.a[2], cal.a[0],
			      cal.a[4], cal.a[5], cal.a[3], cal.a[6],
			      xres, yres);
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
