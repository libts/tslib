/*
 *  tslib/src/ts_test.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: GPL-2.0+
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

static int palette[] = {
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0x304050, 0x80b8c0
};
#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

#define NR_BUTTONS 3
static struct ts_button buttons[NR_BUTTONS];

static void sig(int sig)
{
	close_framebuffer();
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
}

static void refresh_screen(void)
{
	int i;

	fillrect(0, 0, xres - 1, yres - 1, 0);
	put_string_center(xres / 2, yres / 4, "Touchscreen test program", 1);
	put_string_center(xres / 2, yres / 4 + 20,
			  "Touch screen to move crosshair", 2);

	for (i = 0; i < NR_BUTTONS; i++)
		button_draw(&buttons[i]);
}

static void help(void)
{
	ts_print_ascii_logo(16);
	print_version();
	printf("\n");
	printf("Usage: ts_test [-r <rotate_value>] [--version] [--help]\n");
	printf("\n");
	printf("-r --rotate\n");
	printf("        <rotate_value> 0 ... no rotation; 0 degree (default)\n");
	printf("                       1 ... clockwise orientation; 90 degrees\n");
	printf("                       2 ... upside down orientation; 180 degrees\n");
	printf("                       3 ... counterclockwise orientation; 270 degrees\n");
	printf("-h --help\n");
	printf("                       print this help test\n");
	printf("-v --version\n");
	printf("                       print version information only\n");
	printf("\n");
	printf("Example (Linux): ts_test -r $(cat /sys/class/graphics/fbcon/rotate)\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	struct tsdev *ts;
	int x, y;
	unsigned int i;
	unsigned int mode = 0;
	int quit_pressed = 0;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "rotate",       required_argument, NULL, 'r' },
			{ "version",      no_argument,       NULL, 'v' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hvr:", long_options, &option_index);

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
		perror("ts_open");
		exit(1);
	}

	if (open_framebuffer()) {
		close_framebuffer();
		ts_close(ts);
		exit(1);
	}

	x = xres / 2;
	y = yres / 2;

	for (i = 0; i < NR_COLORS; i++)
		setcolor(i, palette[i]);

	/* Initialize buttons */
	memset(&buttons, 0, sizeof(buttons));
	buttons[0].w = buttons[1].w = buttons[2].w = xres / 4;
	buttons[0].h = buttons[1].h = buttons[2].h = 20;
	buttons[0].x = 0;
	buttons[1].x = (3 * xres) / 8;
	buttons[2].x = (3 * xres) / 4;
	buttons[0].y = buttons[1].y = buttons[2].y = 10;
	buttons[0].text = "Drag";
	buttons[1].text = "Draw";
	buttons[2].text = "Quit";

	refresh_screen();

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
			close_framebuffer();
			ts_close(ts);
			exit(1);
		}

		if (ret != 1)
			continue;

		for (i = 0; i < NR_BUTTONS; i++)
			if (button_handle(&buttons[i], samp.x, samp.y, samp.pressure))
				switch (i) {
				case 0:
					mode = 0;
					refresh_screen();
					break;
				case 1:
					mode = 1;
					refresh_screen();
					break;
				case 2:
					quit_pressed = 1;
				}

		printf("%ld.%06ld: %6d %6d %6d\n", samp.tv.tv_sec, samp.tv.tv_usec,
			samp.x, samp.y, samp.pressure);

		if (samp.pressure > 0) {
			if (mode == 0x80000001)
				line(x, y, samp.x, samp.y, 2);
			x = samp.x;
			y = samp.y;
			mode |= 0x80000000;
		} else
			mode &= ~0x80000000;
		if (quit_pressed)
			break;
	}
	fillrect(0, 0, xres - 1, yres - 1, 0);
	close_framebuffer();
	ts_close(ts);

	return 0;
}
