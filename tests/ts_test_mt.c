/*
 *  tslib/src/ts_test_mt.c
 *
 *  Copyright (C) 2016 Martin Kepplinger
 *
 * This file is part of tslib.
 *
 * ts_test_mt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * ts_test_mt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ts_test_mt.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 *
 * Basic multitouch test program for the libts library.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#if defined (__FreeBSD__)

#include <dev/evdev/input.h>
#define TS_HAVE_EVDEV

#elif defined (__linux__)

#include <linux/input.h>
#define TS_HAVE_EVDEV

#endif

#ifdef TS_HAVE_EVDEV
#include <sys/ioctl.h>
#endif

#include "tslib.h"
#include "fbutils.h"
#include "testutils.h"

#ifndef ABS_MT_SLOT /* < 2.6.36 kernel headers */
# define ABS_MT_SLOT             0x2f    /* MT slot being modified */
#endif

static int palette[] = {
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0x304050, 0x80b8c0
};
#define NR_COLORS (int)(sizeof (palette) / sizeof (palette[0]))

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
	put_string_center(xres / 2, yres / 4, "Multitouch test program", 1);
	put_string_center(xres / 2, yres / 4 + 20,
			  "Touch screen to move crosshairs", 2);

	for (i = 0; i < NR_BUTTONS; i++)
		button_draw(&buttons[i]);
}

static void help(void)
{
	ts_print_ascii_logo(16);
	print_version();
	printf("\n");
	printf("Usage: ts_test_mt [-v] [-i <device>] [-j <slots>] [-r <rotate_value>]\n");
	printf("\n");
	printf("-i --idev\n");
	printf("                       Override the input device to use\n");
	printf("-j --slots\n");
	printf("                       Override the number of possible touch contacts\n");
	printf("                       Automatically detected only on Linux, but not\n");
	printf("                       for all devices\n");
	printf("-r --rotate\n");
	printf("        <rotate_value> 0 ... no rotation; 0 degree (default)\n");
	printf("                       1 ... clockwise orientation; 90 degrees\n");
	printf("                       2 ... upside down orientation; 180 degrees\n");
	printf("                       3 ... counterclockwise orientation; 270 degrees\n");
	printf("-n --samples\n");
	printf("                       exit automatically after n samples\n");
	printf("-a --altcross\n");
	printf("                       use an alternative crosshair\n");
	printf("-h --help\n");
	printf("                       print this help text\n");
	printf("-v --verbose\n");
	printf("                       print the touch samples to stdout\n");
	printf("\n");
	printf("Example (Linux): ts_test_mt -r $(cat /sys/class/graphics/fbcon/rotate)\n");
	printf("\n");
}

void print_slot_info(char *slot_info, size_t len, int32_t max_slots,
		     int32_t user_slots)
{
	snprintf(slot_info, len,
		 "%d touch contacts supported (%s)",
		 max_slots, user_slots ? "user" : "driver");
	put_string(2, yres - 14, slot_info, 2);
}

#define CROSS_VISIBLE 0x00001000
#define CROSS_SHOW 0x00000008
#define DRAWING 0x80000000

int main(int argc, char **argv)
{
	struct tsdev *ts;
	int *x = NULL;
	int *y = NULL;
	int i, j;
	unsigned int mode = 0;
	unsigned int *mode_mt = NULL;
	int quit_pressed = 0;
#ifdef TS_HAVE_EVDEV
	struct input_absinfo slot;
#endif
	int32_t user_slots = 0;
	int32_t max_slots = 1;
	struct ts_sample_mt **samp_mt = NULL;
	short verbose = 0;

	const char *tsdevice = NULL;
	char slot_info[64];
	uint32_t samples = 0;
	uint32_t have_samples = 0;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "verbose",      no_argument,       NULL, 'v' },
			{ "idev",         required_argument, NULL, 'i' },
			{ "slots",        required_argument, NULL, 'j' },
			{ "rotate",       required_argument, NULL, 'r' },
			{ "samples",      required_argument, NULL, 'n' },
			{ "altcross",     required_argument, NULL, 'a' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hi:vj:r:n:a:", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			help();
			return 0;

		case 'v':
			verbose = 1;
			break;

		case 'i':
			tsdevice = optarg;
			break;

		case 'j':
			user_slots = atoi(optarg);
			if (user_slots <= 0) {
				help();
				return 0;
			}
			break;

		case 'n':
			samples = atoi(optarg);
			break;

		case 'a':
			alternative_cross = atoi(optarg);
			if (alternative_cross < 0 || alternative_cross > 1) {
				help();
				return 0;
			}
			break;

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

	ts = ts_setup(tsdevice, 0);
	if (!ts) {
		perror("ts_setup");
		return errno;
	}
	if (verbose && tsdevice)
		printf("ts_test_mt: using input device " GREEN "%s" RESET "\n", tsdevice);
#ifdef TS_HAVE_EVDEV
	if (ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
		perror("ioctl EVIOGABS");
		ts_close(ts);
		return errno;
	}

	max_slots = slot.maximum + 1 - slot.minimum;
#endif
	if (user_slots > 0)
		max_slots = user_slots;

	samp_mt = malloc(sizeof(struct ts_sample_mt *));
	if (!samp_mt) {
		ts_close(ts);
		return -ENOMEM;
	}
	samp_mt[0] = calloc(max_slots, sizeof(struct ts_sample_mt));
	if (!samp_mt[0]) {
		free(samp_mt);
		ts_close(ts);
		return -ENOMEM;
	}

	if (open_framebuffer()) {
		close_framebuffer();
		free(samp_mt[0]);
		free(samp_mt);
		ts_close(ts);
		exit(1);
	}

	x = calloc(max_slots, sizeof(int));
	if (!x)
		goto out;

	y = calloc(max_slots, sizeof(int));
	if (!y)
		goto out;

	mode_mt = calloc(max_slots, sizeof(unsigned int));
	if (!mode_mt)
		goto out;

	for (i = 0; i < max_slots; i++) {
		x[i] = xres / 2;
		y[i] = yres / 2;
	}

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
	print_slot_info(slot_info, sizeof(slot_info), max_slots, user_slots);

	while (1) {
		int ret;

		if (samples) {
			if (samples >= have_samples)
				goto out;

			have_samples++;
		}

		/* Show the cross */
		for (j = 0; j < max_slots; j++) {
			if ((mode & 15) != 1) { /* not in draw mode */
				/* Hide slots > 0 if released */
				if (j > 0 && !(mode_mt[j] & CROSS_SHOW))
					continue;

				if (!(mode_mt[j] & CROSS_VISIBLE))
					put_cross(x[j], y[j], 2 | XORMODE);

				mode_mt[j] |= CROSS_VISIBLE;
			}
		}

		ret = ts_read_mt(ts, samp_mt, max_slots, 1);

		/* Hide it */
		for (j = 0; j < max_slots; j++) {
			if ((mode & 15) != 1) { /* not in draw mode */
				if (j > 0 && !(mode_mt[j] & CROSS_SHOW) &&
				    !(mode_mt[j] & CROSS_VISIBLE))
					continue;

				put_cross(x[j], y[j], 2 | XORMODE);

				mode_mt[j] &= ~CROSS_VISIBLE;
			}
		}

		if (ret < 0) {
			perror("ts_read_mt");
			close_framebuffer();
			free(samp_mt);
			ts_close(ts);
			exit(1);
		}

		if (ret != 1)
			continue;

		for (j = 0; j < max_slots; j++) {
			if (!(samp_mt[0][j].valid & TSLIB_MT_VALID))
				continue;

			for (i = 0; i < NR_BUTTONS; i++) {
				if (button_handle(&buttons[i],
						  samp_mt[0][j].x,
						  samp_mt[0][j].y,
						  samp_mt[0][j].pressure)) {
					switch (i) {
					case 0:
						mode = 0x0;
						refresh_screen();
						print_slot_info(slot_info,
								sizeof(slot_info),
								max_slots,
								user_slots);
						break;
					case 1:
						mode = 0x1;
						refresh_screen();
						print_slot_info(slot_info,
								sizeof(slot_info),
								max_slots,
								user_slots);
						break;
					case 2:
						quit_pressed = 1;
					}
				}
			}

			if (verbose) {
				printf(YELLOW "%ld.%06ld:" RESET " (slot %d) %6d %6d %6d\n",
					samp_mt[0][j].tv.tv_sec,
					samp_mt[0][j].tv.tv_usec,
					samp_mt[0][j].slot,
					samp_mt[0][j].x,
					samp_mt[0][j].y,
					samp_mt[0][j].pressure);
			}

			if (samp_mt[0][j].pressure > 0) {
				if (mode & DRAWING && mode & 0x1) { /* draw mode while drawing */
					if (mode_mt[j] & DRAWING) /* slot while drawing */
						line(x[j], y[j], samp_mt[0][j].x, samp_mt[0][j].y, 2);
				}
				x[j] = samp_mt[0][j].x;
				y[j] = samp_mt[0][j].y;
				mode |= DRAWING;
				mode_mt[j] |= DRAWING;
				mode_mt[j] |= CROSS_SHOW;
			} else {
				mode &= ~DRAWING;
				mode_mt[j] &= ~DRAWING;
				mode_mt[j] &= ~CROSS_SHOW; /* hide the cross */
			}
			if (quit_pressed)
				goto out;
		}
	}

out:
	fillrect(0, 0, xres - 1, yres - 1, 0);
	close_framebuffer();

	if (ts)
		ts_close(ts);

	if (samp_mt) {
		free(samp_mt[0]);
		free(samp_mt);
	}

	free(x);
	free(y);
	free(mode_mt);

	return 0;
}
