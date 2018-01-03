/*
 *  tslib/src/ts_test.c
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
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <errno.h>

#include "tslib.h"
#include "fbutils.h"
#include "testutils.h"

static int palette [] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0x304050, 0x80b8c0
};
#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

#define SQOFF 		50
#define SQSZ		5

#define NR_BUTTONS 10
static int nr_buttons = 0;
static struct ts_button buttons [NR_BUTTONS];
static struct tsdev *ts = NULL;

static void print_usage() {
	printf("Usage: ts_test [-v] [-r] [-h]\n"
			"  -v  HED Verify Calibration (for use at command line)\n"
			"  -r  HED same as -v but shows Redo Calibration button (for use in scripts)\n"
			"  -h  Print out this help message\n\n");
}

static void sig(int sig)
{
	if(ts)
		ts_close(ts);
	close_framebuffer();
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
}

static void refresh_screen(void)
{
	int i;

	fillrect (0, 0, xres - 1, yres - 1, 0);
	put_string_center (xres/2, yres/4,   "Touchscreen test program", 1);
	put_string_center (xres/2, yres/4+20,"Touch screen to move crosshair", 2);

	for (i = 0; i < nr_buttons; i++)
		button_draw (&buttons [i]);
}

static void help(void)
{
	struct ts_lib_version_data *ver = ts_libversion();

	printf("tslib %s (library 0x%X)\n", ver->package_version, ver->version_num);
	printf("\n");
	printf("Usage: ts_test [-r <rotate_value>]\n");
	printf("\n");
	printf("        <rotate_value> 0 ... no rotation; 0 degree (default)\n");
	printf("                       1 ... clockwise orientation; 90 degrees\n");
	printf("                       2 ... upside down orientation; 180 degrees\n");
	printf("                       3 ... counterclockwise orientation; 270 degrees\n");
	printf("\n");
	printf("Example (Linux): ts_test -r $(cat /sys/class/graphics/fbcon/rotate)\n");
	printf("\n");
}

static void refresh_screen_hed ()
{
	int i;
	int ystep;
	char xtext[64] = {0};
	char ytext[64] = {0};

	fillrect (0, 0, xres - 1, yres - 1, 0);

	#define LINE_SPACING 20

	/*top text*/
	ystep = buttons[1].h + LINE_SPACING * 1.5;
	put_string_center (xres/2, ystep,   "TOUCH SCREEN CALIBRATION VERIFICATION", 1);
	ystep+=LINE_SPACING;
	put_string_center (xres/2, ystep,"Touch the stylus on each of the 5 small boxes and", 2);
	ystep+=LINE_SPACING;
	put_string_center (xres/2, ystep,"verify that the crosshairs are also inside that box.", 2);
	
	/*bottom text (last line first)*/
	ystep = buttons[0].y - LINE_SPACING * 1.5;
	put_string_center (xres/2, ystep,"crosshairs to touch the box, you MUST REDO CALIBRATION.", 2);
	ystep-=LINE_SPACING;
	put_string_center (xres/2, ystep,"IMPORTANT: If you have to move off of the box to get the", 2);

	for (i = 0; i < nr_buttons; i++) {
		button_draw (&buttons [i]);
		if(i > 1) {
			snprintf(xtext, 63, "X%d", buttons[i].x + SQSZ);
			snprintf(ytext, 63, "Y%d", buttons[i].y + SQSZ);
			put_string_center (buttons[i].x - 25 + SQSZ, buttons[i].y + 25 + SQSZ, xtext, 1 | XORMODE);
			put_string_center (buttons[i].x + 25 + SQSZ, buttons[i].y + 25 + SQSZ, ytext, 1 | XORMODE);
		}
	}
}


int ts_test(void)
{

	int x, y;
	unsigned int i;
	unsigned int mode = 0;
	int quit_pressed = 0;

	x = xres/2;
	y = yres/2;

	for (i = 0; i < NR_COLORS; i++)
		setcolor (i, palette [i]);

	/* Initialize buttons */
	nr_buttons = 3;
	memset (&buttons, 0, sizeof (buttons));
	buttons [0].w = buttons [1].w = buttons [2].w = xres / 4;
	buttons [0].h = buttons [1].h = buttons [2].h = 20;
	buttons [0].x = 0;
	buttons [1].x = (3 * xres) / 8;
	buttons [2].x = (3 * xres) / 4;
	buttons [0].y = buttons [1].y = buttons [2].y = 10;
	buttons [0].text = "Drag";
	buttons [1].text = "Draw";
	buttons [2].text = "Quit";

	refresh_screen ();

	while (1) {
		struct ts_sample samp;
		int ret;

		/* Show the cross */
		if ((mode & 15) != 1)
			put_cross(x, y, 2 | XORMODE);

		ret = ts_read(ts, &samp, 1);

		/* Hide it */
		if ((mode & 15) != 1)
			put_cross(x, y, 2 | XORMODE);

		if (ret < 0) {
			perror("ts_read");
			if(ts)
				ts_close(ts);
			close_framebuffer();
			ts_close(ts);
			exit(1);
		}

		if (ret != 1)
			continue;

		for (i = 0; i < nr_buttons; i++)
			if (button_handle(&buttons [i], samp.x, samp.y, samp.pressure))
				switch (i) {
				case 0:
					mode = 0;
					refresh_screen ();
					break;
				case 1:
					mode = 1;
					refresh_screen ();
					break;
				case 2:
					quit_pressed = 1;
				}

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec,
			samp.x, samp.y, samp.pressure);

		if (samp.pressure > 0) {
			if (mode == 0x80000001)
				line (x, y, samp.x, samp.y, 2);
			x = samp.x;
			y = samp.y;
			mode |= 0x80000000;
		} else
			mode &= ~0x80000000;
		if (quit_pressed)
			break;
	}
	
	return 0;
}

int hed_test(int verifycalibration)
{
	int retval = 0;
	int x, y;
	unsigned int i;
	unsigned int mode = 0;
	int quit_pressed = 0;

	int crosshairs_show;
	int crosshairs_showing;
	char xtext[64] = {0};
	char ytext[64] = {0};


	for (i = 0; i < NR_COLORS; i++)
		setcolor (i, palette [i]);

	/* Initialize buttons */
	nr_buttons = 7;
	memset (&buttons, 0, sizeof (buttons));
	buttons [0].w = buttons [1].w = xres / 4;
	buttons [0].h = buttons [1].h = 40;
	
	buttons [0].x = (xres - buttons [0].w) / 2;
	buttons [1].x = (xres - buttons [1].w) / 2;
	
	buttons [0].y = yres - buttons [0].h - 1;
	buttons [1].y = 0;

	if(verifycalibration & 0x2) {
		buttons [0].text = "REDO CALIBRATION";
		buttons [1].text = "PASS CALIBRATION";
	} else {
		buttons [0].text = "QUIT";
		buttons [1].text = "QUIT";
	}
	/*draw buttons at the 5 orig cal points*/
	buttons[2].x = SQOFF - SQSZ;
	buttons[2].y = SQOFF - SQSZ;
	buttons[2].w = SQSZ * 2 + 1;
	buttons[2].h = SQSZ * 2 + 1;
	buttons[2].text = "";
	
	buttons[3].x = xres - SQOFF - SQSZ;
	buttons[3].y = SQOFF - SQSZ;
	buttons[3].w = SQSZ * 2 + 1;
	buttons[3].h = SQSZ * 2 + 1;
	buttons[3].text = "";

	buttons[4].x = SQOFF - SQSZ;
	buttons[4].y = yres - SQOFF - SQSZ;
	buttons[4].w = SQSZ * 2 + 1;
	buttons[4].h = SQSZ * 2 + 1;
	buttons[4].text = "";

	buttons[5].x = xres - SQOFF - SQSZ;
	buttons[5].y = yres - SQOFF - SQSZ;
	buttons[5].w = SQSZ * 2 + 1;
	buttons[5].h = SQSZ * 2 + 1;
	buttons[5].text = "";

	buttons[6].x = xres / 2 - SQSZ;
	buttons[6].y = yres / 2 - SQSZ;
	buttons[6].w = SQSZ * 2 + 1;
	buttons[6].h = SQSZ * 2 + 1;
	buttons[6].text = "";

	refresh_screen_hed ();

	crosshairs_show = 0;
	crosshairs_showing = 0;
	x = 0;
	y = 0;
	
	while (1) {
		struct ts_sample samp;
		int ret;

		/* Show the cross */
		if(crosshairs_show && !crosshairs_showing) {
			put_crosshairs(x, y, 2 | XORMODE);
			snprintf(xtext, 63, "X%d", x);
			snprintf(ytext, 63, "Y%d", y);
			put_string_center (x-25, y-25, xtext, 1 | XORMODE);
			put_string_center (x+25, y-25, ytext, 1 | XORMODE);
			crosshairs_showing = 1;
		}

		ret = ts_read(ts, &samp, 1);

		if (ret < 0) {
			perror("ts_read");
			close_framebuffer();
			exit(1);
		}

		/* Hide it */
		if(crosshairs_showing) {
			put_crosshairs(x, y, 2 | XORMODE);
			put_string_center (x-25, y-25, xtext, 1 | XORMODE);
			put_string_center (x+25, y-25, ytext, 1 | XORMODE);
			crosshairs_showing = 0;
		}

		if (ret != 1)
			continue;

		for (i = 0; i < nr_buttons; i++) {
			if (button_handle(&buttons [i], samp.x, samp.y, samp.pressure)) {
				switch (i) {
				case 0:
					if(verifycalibration & 0x2) {
						retval = 1; /*calibration fail*/
					} else {
						retval = 0; /*normal exit*/
					}
					quit_pressed = 1; 
					break;
				case 1:
					retval = 0;	/*calibration pass*/
					quit_pressed = 1; 
					break;
				}
			}
		}
		
		if (samp.pressure > 0) {
			x = samp.x;
			y = samp.y;
			crosshairs_show = 1;
		} else {
			crosshairs_show = 0;
		}
		
		if (quit_pressed)
			break;
	}
	
	return retval;
}

int main(int argc, char **argv)
{
	int option = 0;
	int verifycalibration = 0;
	char *tsdevice=NULL;
	int retval = 0;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	while (1) { 
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "rotate",       no_argument,       NULL, 'r' },
			{ "verify",       no_argument,       NULL, 'v' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hrv", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'v':
			verifycalibration |= 1; /* |= 1 just in case they put the v after the r*/
			break;
		case 'h':
			help();
			return 0;
		case 'r':
			verifycalibration = 3; /*assumes v option*/
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
		perror("ts_open");
		exit(1);
	}

	if (open_framebuffer()) {
		close_framebuffer();
		ts_close(ts);
		exit(1);
	}
	        
	if (verifycalibration) {
		retval = hed_test(verifycalibration);
	} else {
		retval = ts_test();
	}

	fillrect(0, 0, xres - 1, yres - 1, 0);
	close_framebuffer();
	ts_close(ts);

	return retval;
}
