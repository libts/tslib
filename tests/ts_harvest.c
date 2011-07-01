/*
 *  tslib/src/ts_harvest.c
 *
 *  Copyright (C) 2004 Michael Opdenacker <michaelo@handhelds.org>
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Program to harvest hundreds of raw touchscreen coordinates
 * Useful for linearizing non linear touchscreens as found in the h2200 device
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "tslib.h"
#include "fbutils.h"
#include "testutils.h"

static int palette [] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0x304050, 0x80b8c0
};

#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

/* MINIMUM X AND Y STEPS
 */

#define X_STEP 10
#define Y_STEP 10

static void sig(int sig)
{
	close_framebuffer();
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
}

static void refresh_screen ()
{
	fillrect (0, 0, xres - 1, yres - 1, 0);
}

static void ts_harvest_put_cross (int x, int y, unsigned colidx)
{
        line (x - 10, y, x - 2, y, colidx);
        line (x + 2, y, x + 10, y, colidx);
        line (x, y - 10, x, y - 2, colidx);
        line (x, y + 2, x, y + 10, colidx);
}                                                                                                              
int main()
{
	struct tsdev *ts;
	int x_ts, y_ts, x_incr, y_incr;
	unsigned int x, y, xres_half, yres_half, x_new, y_new;
	unsigned int i;
	char *tsdevice=NULL;
	FILE *output_fid;

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

	refresh_screen ();
	put_string_center (xres/2, yres*0.1, "TSLIB harvesting utility", 4);
	put_string_center (xres/2, yres*0.15, "Touch the crosshair center", 1);
	put_string_center (xres/2, yres*0.2, "with as much accurary", 1);
	put_string_center (xres/2, yres*0.25, "as possible", 1);
	put_string_center (xres/2, yres*0.35, "Touch anywhere to start", 1);

	getxy (ts, &x_ts, &y_ts); 
	refresh_screen ();

	output_fid = fopen ("ts_harvest.out", "w");
	fprintf (output_fid, "X_expected\tY_expected\tX_measured\tY_measured\n");

	y = 0;
	y_incr = 1;
	xres_half = xres / 2;
	yres_half = yres / 2;

	while (y < yres && y_incr >= 0)  {

	   x = 0;
	   x_incr = 1;

	   while (x < xres && x_incr >= 0) {

		/* Leave time for the user to move the pen up
		 * otherwise the pen may still be down on the previous location
		 * while a new cross has already been drawn.
		 */
	
		usleep (700000);

		/* Show the cross */
		ts_harvest_put_cross(x, y, 2 | XORMODE);

		/* Flush the touchscreen */

		ts_flush (ts);

		/* Leave time for the user to see and touch the new location.
		 * If the pen was still down, (s)he will see that the
		 * cross has moved. Otherwise, touchscreen data may
		 * immediately be recorded and the user won't even have
		 * time to see the cross.
		 */

		usleep (700000);

		/* Get a point */
		
		getxy (ts, &x_ts, &y_ts);
        	fprintf (output_fid, "%d\t%d\t%d\t%d\n", x, y, x_ts, y_ts);

		/* Hide the cross */

		ts_harvest_put_cross(x, y, 2 | XORMODE);

		/* Manage increments */

		x_new = x + (x_incr * X_STEP); 

		if (x < xres_half) {
		   if (x_new > xres_half) {
		      x_new = xres_half + (xres_half - x - 1);
		      x_incr--;
		   } else if (x_new < xres_half) {
		      x_incr++;
		   }
		} else {
		   x_incr--;
		}

		x = x_new;
	   }

	   y_new = y + (y_incr * Y_STEP); 

	   if (y < yres_half) {
              if  (y_new > yres_half) {
                  y_new = yres_half + (yres_half - y - 1);
                  y_incr--;
              } else if (y_new < yres_half) { 
                  y_incr++;
              }
           } else {
              y_incr--;
           }
                                                                                                              
           y = y_new;  
	}

	fclose (output_fid);

	refresh_screen ();
	put_string_center (xres/2, yres*0.75, "Thank you (pfooh!)", 1);
	put_string_center (xres/2, yres*0.80, "Output saved to ts_harvest.out", 1);
	put_string_center (xres/2, yres*0.85, "Touch anywhere to quit", 4);
	getxy (ts, &x_ts, &y_ts); 
	refresh_screen ();
	close_framebuffer();
	return 0;
}
