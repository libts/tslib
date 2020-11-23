/*
 * (C) 2017 Martin Keppligner <martink@posteo.de>
 *
 * This file is part of tslib.
 *
 * ts_calibrate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * ts_calibrate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this tool.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <stdint.h>

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

#include <tslib.h>
#include "sdlutils.h"

static void help(void)
{
	ts_print_ascii_logo(16);
	printf("%s", tslib_version());
	printf("\n");
	printf("Usage: ts_test_mt [-v] [-i <device>] [-j <slots>]\n");
	printf("\n");
	printf("        <device>       Override the input device to use\n");
	printf("        <slots>        Override the number of possible touch contacts\n");
	printf("                       Automatically detected only on Linux, but not\n");
	printf("                       for all devices\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	struct tsdev *ts;
#ifdef TS_HAVE_EVDEV
	struct input_absinfo slot;
#endif
	int32_t user_slots = 0;
	int32_t max_slots = 1;
	const char *tsdevice = NULL;
	struct ts_sample_mt **samp_mt = NULL;
	short verbose = 0;
	int ret;
	int i;
	SDL_Window *sdlWindow;
	SDL_Renderer *sdlRenderer;
	SDL_Event ev;

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "verbose",      no_argument,       NULL, 'v' },
			{ "idev",         required_argument, NULL, 'i' },
			{ "slots",        required_argument, NULL, 'j' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hi:vj:", long_options, &option_index);

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

	SDL_SetMainReady();

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "Couldn't initialize SDL: %s", SDL_GetError());
		goto out;
	}

	if (SDL_CreateWindowAndRenderer(0, 0,
					SDL_WINDOW_FULLSCREEN_DESKTOP,
					&sdlWindow, &sdlRenderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "Couldn't create window and renderer: %s",
			     SDL_GetError());
		goto out;
	}

	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlRenderer);

	while (1) {
		ret = ts_read_mt(ts, samp_mt, max_slots, 1);
		if (ret < 0) {
			SDL_Quit();
			goto out;
		}

		if (ret != 1)
			continue;

		SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
		SDL_RenderClear(sdlRenderer);

		for (i = 0; i < max_slots; i++) {
			if (!(samp_mt[0][i].valid & TSLIB_MT_VALID))
				continue;

			draw_crosshair(sdlRenderer,
				       samp_mt[0][i].x, samp_mt[0][i].y);

			if (verbose) {
				printf("%ld.%06ld: (slot %d) %6d %6d %6d\n",
					samp_mt[0][i].tv.tv_sec,
					samp_mt[0][i].tv.tv_usec,
					samp_mt[0][i].slot,
					samp_mt[0][i].x,
					samp_mt[0][i].y,
					samp_mt[0][i].pressure);
			}
		}

		SDL_PollEvent(&ev);
		switch (ev.type) {
		case SDL_KEYDOWN:
		case SDL_QUIT:
			SDL_ShowCursor(SDL_ENABLE);
			SDL_DestroyWindow(sdlWindow);
			SDL_Quit();
			goto out;
		}

		SDL_RenderPresent(sdlRenderer);
	}
out:
	if (ts)
		ts_close(ts);

	if (samp_mt) {
		free(samp_mt[0]);
		free(samp_mt);
	}
	return 0;
}
