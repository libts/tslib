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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/fb.h>

#include "tslib.h"

#include "fbutils.h"
#include "testutils.h"

static int palette [] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0
};
#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

typedef struct {
	int x[5], xfb[5];
	int y[5], yfb[5];
	int a[7];
} calibration;

static void sig(int sig)
{
	close_framebuffer ();
	fflush (stderr);
	printf ("signal %d caught\n", sig);
	fflush (stdout);
	exit (1);
}

int perform_calibration(calibration *cal) {
	int j;
	float n, x, y, x2, y2, xy, z, zx, zy;
	float det, a, b, c, e, f, i;
	float scaling = 65536.0;

// Get sums for matrix
	n = x = y = x2 = y2 = xy = 0;
	for(j=0;j<5;j++) {
		n += 1.0;
		x += (float)cal->x[j];
		y += (float)cal->y[j];
		x2 += (float)(cal->x[j]*cal->x[j]);
		y2 += (float)(cal->y[j]*cal->y[j]);
		xy += (float)(cal->x[j]*cal->y[j]);
	}

// Get determinant of matrix -- check if determinant is too small
	det = n*(x2*y2 - xy*xy) + x*(xy*y - x*y2) + y*(x*xy - y*x2);
	if(det < 0.1 && det > -0.1) {
		printf("ts_calibrate: determinant is too small -- %f\n",det);
		return 0;
	}

// Get elements of inverse matrix
	a = (x2*y2 - xy*xy)/det;
	b = (xy*y - x*y2)/det;
	c = (x*xy - y*x2)/det;
	e = (n*y2 - y*y)/det;
	f = (x*y - n*xy)/det;
	i = (n*x2 - x*x)/det;

// Get sums for x calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->xfb[j];
		zx += (float)(cal->xfb[j]*cal->x[j]);
		zy += (float)(cal->xfb[j]*cal->y[j]);
	}

// Now multiply out to get the calibration for framebuffer x coord
	cal->a[0] = (int)((a*z + b*zx + c*zy)*(scaling));
	cal->a[1] = (int)((b*z + e*zx + f*zy)*(scaling));
	cal->a[2] = (int)((c*z + f*zx + i*zy)*(scaling));

	printf("%f %f %f\n",(a*z + b*zx + c*zy),
				(b*z + e*zx + f*zy),
				(c*z + f*zx + i*zy));

// Get sums for y calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->yfb[j];
		zx += (float)(cal->yfb[j]*cal->x[j]);
		zy += (float)(cal->yfb[j]*cal->y[j]);
	}

// Now multiply out to get the calibration for framebuffer y coord
	cal->a[3] = (int)((a*z + b*zx + c*zy)*(scaling));
	cal->a[4] = (int)((b*z + e*zx + f*zy)*(scaling));
	cal->a[5] = (int)((c*z + f*zx + i*zy)*(scaling));

	printf("%f %f %f\n",(a*z + b*zx + c*zy),
				(b*z + e*zx + f*zy),
				(c*z + f*zx + i*zy));

// If we got here, we're OK, so assign scaling to a[6] and return
	cal->a[6] = (int)scaling;
	return 1;
/*	
// This code was here originally to just insert default values
	for(j=0;j<7;j++) {
		c->a[j]=0;
	}
	c->a[1] = c->a[5] = c->a[6] = 1;
	return 1;
*/

}

static void get_sample (struct tsdev *ts, calibration *cal,
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
			put_cross (last_x >> 16, last_y >> 16, 2 | XORMODE);
			usleep (1000);
			put_cross (last_x >> 16, last_y >> 16, 2 | XORMODE);
			last_x += dx;
			last_y += dy;
		}
	}

	put_cross(x, y, 2 | XORMODE);
	getxy (ts, &cal->x [index], &cal->y [index]);
	put_cross(x, y, 2 | XORMODE);

	last_x = cal->xfb [index] = x;
	last_y = cal->yfb [index] = y;

	printf("%s : X = %4d Y = %4d\n", name, cal->x [index], cal->y [index]);
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
		if (nfds == 0) break;

		if (ts_read_raw(ts, &sample, 1) < 0) {
			perror("ts_read");
			exit(1);
		}
	}
}

int main()
{
	struct tsdev *ts;
	calibration cal;
	int cal_fd;
	char cal_buffer[256];
	char *tsdevice = NULL;
	char *calfile = NULL;
	unsigned int i, len;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	if( (tsdevice = getenv("TSLIB_TSDEVICE")) != NULL ) {
		ts = ts_open(tsdevice,0);
	} else {
		if (!(ts = ts_open("/dev/input/event0", 0)))
			ts = ts_open("/dev/touchscreen/ucb1x00", 0);
	}

	if (!ts) {
		perror("ts_open");
		exit(1);
	}
	if (ts_config(ts)) {
		perror("ts_config");
		exit(1);
	}

	if (open_framebuffer()) {
		close_framebuffer();
		exit(1);
	}

	for (i = 0; i < NR_COLORS; i++)
		setcolor (i, palette [i]);

	put_string_center (xres / 2, yres / 4,
			   "TSLIB calibration utility", 1);
	put_string_center (xres / 2, yres / 4 + 20,
			   "Touch crosshair to calibrate", 2);

	printf("xres = %d, yres = %d\n", xres, yres);

	// Clear the buffer
	clearbuf(ts);

	get_sample (ts, &cal, 0, 50,        50,        "Top left");
	clearbuf(ts);
	get_sample (ts, &cal, 1, xres - 50, 50,        "Top right");
	clearbuf(ts);
	get_sample (ts, &cal, 2, xres - 50, yres - 50, "Bot right");
	clearbuf(ts);
	get_sample (ts, &cal, 3, 50,        yres - 50, "Bot left");
	clearbuf(ts);
	get_sample (ts, &cal, 4, xres / 2,  yres / 2,  "Center");

	if (perform_calibration (&cal)) {
		printf ("Calibration constants: ");
		for (i = 0; i < 7; i++) printf("%d ", cal.a [i]);
		printf("\n");
		if ((calfile = getenv("TSLIB_CALIBFILE")) != NULL) {
			cal_fd = open (calfile, O_CREAT | O_TRUNC | O_RDWR,
			               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		} else {
			cal_fd = open (TS_POINTERCAL, O_CREAT | O_TRUNC | O_RDWR,
			               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		}
		len = sprintf(cal_buffer,"%d %d %d %d %d %d %d %d %d",
		              cal.a[1], cal.a[2], cal.a[0],
		              cal.a[4], cal.a[5], cal.a[3], cal.a[6],
		              xres, yres);
		write (cal_fd, cal_buffer, len);
		close (cal_fd);
                i = 0;
	} else {
		printf("Calibration failed.\n");
		i = -1;
	}

	close_framebuffer();
	return i;
}
