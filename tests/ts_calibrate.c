/*
 *  tslib/tests/ts_calibrate.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_calibrate.c,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Basic test program for touchscreen library.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>

#include "tslib.h"

static int con_fd, fb_fd, last_vt = -1;
static struct fb_fix_screeninfo fix;
static struct fb_var_screeninfo var;
static struct fb_cmap cmap;
static char *fbuffer;

static int open_framebuffer(void)
{
	struct vt_stat vts;
	char vtname[16];
	int fd, nr;
	unsigned short col[2];

	fd = open("/dev/tty1", O_WRONLY);
	if (fd < 0) {
		perror("open /dev/tty1");
		return -1;
	}

	if (ioctl(fd, VT_OPENQRY, &nr) < 0) {
		perror("ioctl VT_OPENQRY");
		return -1;
	}
	close(fd);

	sprintf(vtname, "/dev/tty%d", nr);

	con_fd = open(vtname, O_RDWR | O_NDELAY);
	if (con_fd < 0) {
		perror("open tty");
		return -1;
	}

	if (ioctl(con_fd, VT_GETSTATE, &vts) == 0)
		last_vt = vts.v_active;

	if (ioctl(con_fd, VT_ACTIVATE, nr) < 0) {
		perror("VT_ACTIVATE");
		close(con_fd);
		return -1;
	}

	if (ioctl(con_fd, VT_WAITACTIVE, nr) < 0) {
		perror("VT_WAITACTIVE");
		close(con_fd);
		return -1;
	}

	if (ioctl(con_fd, KDSETMODE, KD_GRAPHICS) < 0) {
		perror("KDSETMODE");
		close(con_fd);
		return -1;
	}

	fb_fd = open("/dev/fb0", O_RDWR);
	if (fb_fd == -1) {
		perror("open /dev/fb");
		return -1;
	}

	if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix) < 0) {
		perror("ioctl FBIOGET_FSCREENINFO");
		close(fb_fd);
		return -1;
	}

	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var) < 0) {
		perror("ioctl FBIOGET_VSCREENINFO");
		close(fb_fd);
		return -1;
	}

	cmap.start = 0;
	cmap.len = 2;
	cmap.red = col;
	cmap.green = col;
	cmap.blue = col;
	cmap.transp = NULL;

	col[0] = 0;
	col[1] = 0xffff;

	if (ioctl(fb_fd, FBIOGETCMAP, &cmap) < 0) {
		perror("ioctl FBIOGETCMAP");
		close(fb_fd);
		return -1;
	}

	fbuffer = mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fb_fd, 0);
	if (fbuffer == (char *)-1) {
		perror("mmap framebuffer");
		close(fb_fd);
		return -1;
	}
	return 0;
}

static void close_framebuffer(void)
{
	munmap(fbuffer, fix.smem_len);
	close(fb_fd);

	if (ioctl(con_fd, KDSETMODE, KD_TEXT) < 0)
		perror("KDSETMODE");

	if (last_vt >= 0)
		if (ioctl(con_fd, VT_ACTIVATE, last_vt))
			perror("VT_ACTIVATE");

	close(con_fd);
}

static void sig(int sig)
{
	close_framebuffer();
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
}

static void put_cross(int x, int y, int c)
{
	int off = y * fix.line_length + x * var.bits_per_pixel / 8;
	int i;

	for (i = 0; i < 50; i++) {
		fbuffer[off + i * fix.line_length + 50] = c;
		fbuffer[off + i * fix.line_length + 51] = c;
	}

	for (i = 0; i < 100; i++)
		fbuffer[off + 25 * fix.line_length + i] = c;
}

static int getxy(struct tsdev *ts, int *x, int *y)
{
	struct ts_sample samp, sa;

	do {
		sa = samp;
		if (ts_read_raw(ts, &samp, 1) < 0) {
			perror("ts_read");
			close_framebuffer();
			exit(1);
		}
	} while (samp.pressure > 100);

	if (x && y) {
		*x = sa.x;
		*y = sa.y;
	}
}

int main()
{
	struct tsdev *ts;
	int fd, x[4], y[4];

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	ts = ts_open("/dev/touchscreen/ucb1x00", 0);
	if (!ts) {
		perror("ts_open");
		exit(1);
	}

	if (open_framebuffer()) {
		close_framebuffer();
		exit(1);
	}

	memset(fbuffer, 0, fix.smem_len);

	getxy(ts, 0, 0);
	put_cross(0,0,0xff);
	getxy(ts, &x[0], &y[0]);
	put_cross(0,0,0);

	printf("Top left : X = %4d Y = %4d\n", x[0], y[0]);

	put_cross(var.xres - 50, 0, 0xff);
	getxy(ts, &x[1], &y[1]);
	put_cross(var.xres - 50, 0, 0);

	printf("Top right: X = %4d Y = %4d\n", x[1], y[1]);

	put_cross(var.xres - 50, var.yres - 50, 0xff);
	getxy(ts, &x[2], &y[2]);
	put_cross(var.xres - 50, var.yres - 50, 0);

	printf("Bot right: X = %4d Y = %4d\n", x[2], y[2]);

	put_cross(0, var.yres - 50, 0xff);
	getxy(ts, &x[3], &y[3]);
	put_cross(0, var.yres - 50, 0);

	printf("Bot left : X = %4d Y = %4d\n", x[3], y[3]);

	printf("X mult = %d div = %d\n",
		var.xres - 50, (x[2] + x[1] - x[0] - x[3]) / 2);

	printf("Y mult = %d div = %d\n",
		var.yres - 50, (y[2] + y[3] - y[0] - y[1]) / 2);

	while (1) {
		struct ts_sample samp;

		if (ts_read_raw(ts, &samp, 1) < 0) {
			perror("ts_read");
			exit(1);
		}

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec,
			samp.x, samp.y, samp.pressure);
	}
}
