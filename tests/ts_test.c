/*
 *  tslib/src/ts_test.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_test.c,v 1.4 2002/07/01 23:02:58 dlowder Exp $
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
#include "fbutils.h"

static void sig(int sig)
{
	close_framebuffer();
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
}

int main()
{
	struct tsdev *ts;
	int x, y;

	char *tsdevice=NULL;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

        if( (tsdevice = getenv("TSLIB_TSDEVICE")) != NULL ) {
                ts = ts_open(tsdevice,0);
        } else {
#ifdef USE_INPUT_API
                ts = ts_open("/dev/input/event0", 0);
#else
                ts = ts_open("/dev/touchscreen/ucb1x00", 0);
#endif /* USE_INPUT_API */
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
	close_framebuffer();
	if (open_framebuffer()) {
		close_framebuffer();
		exit(1);
	}

	x = xres/2;
	y = yres/2;

	setcolors(0x48ff48,0x880000);

	put_string(xres/2,yres/4,"TSLIB test program",1);
	put_string(xres/2,yres/4+20,"Touch screen to move crosshair",1);

	put_cross(x,y,1);

	while (1) {
		struct ts_sample samp;
		int ret;

		ret = ts_read(ts, &samp, 1);

		if (ret < 0) {
			perror("ts_read");
			close_framebuffer();
			exit(1);
		}

		if (ret != 1)
			continue;

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec,
			samp.x, samp.y, samp.pressure);

		if (samp.pressure > 0) {
			put_cross(x, y, 0);
			x = samp.x;
			y = samp.y;
			put_cross(x, y, 1);
		}
	}
}
