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
 */

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <tslib.h>
#include "ts_calibrate.h"

const int BLOCK_SIZE = 9;

static void help(void)
{
	struct ts_lib_version_data *ver = ts_libversion();

	printf("tslib %s (library 0x%X)\n", ver->package_version, ver->version_num);
	printf("\n");
	printf("Usage: ts_calibrate [-v] [-i <device>] [-r <rotate_value>]\n");
	printf("\n");
	printf("        <device>       Override the input device to use\n");
	printf("        <rotate_value> 0 ... no rotation; 0 degree (default)\n");
	printf("                       1 ... clockwise orientation; 90 degrees\n");
	printf("                       2 ... upside down orientation; 180 degrees\n");
	printf("                       3 ... counterclockwise orientation; 270 degrees\n");
	printf("\n");
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
//			put_cross(last_x >> 16, last_y >> 16, 2 | XORMODE);
			usleep(1000);
//			put_cross(last_x >> 16, last_y >> 16, 2 | XORMODE);
			last_x += dx;
			last_y += dy;
		}
	}

//	put_cross(x, y, 2 | XORMODE);
//	getxy(ts, &cal->x[index], &cal->y[index]);
//	put_cross(x, y, 2 | XORMODE);

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

int main(int argc, char **argv)
{
	struct tsdev *ts;
	int32_t max_slots = 1;
	const char *tsdevice = NULL;
	struct ts_sample_mt **samp_mt = NULL;
	short verbose = 0;
	int ret;
	int i;
	SDL_Window *sdlWindow;
	SDL_Renderer *sdlRenderer;
	SDL_Event ev;
	SDL_Rect r;
	calibration cal = {
		.x = { 0 },
		.y = { 0 },
	};
	int xres;
	int yres;
	char *calfile = NULL;
	int cal_fd;
	char cal_buffer[256];
	uint32_t len;

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "verbose",      no_argument,       NULL, 'v' },
			{ "idev",         required_argument, NULL, 'i' },
			{ "rotate",       required_argument, NULL, 'r' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hi:vr:", long_options, &option_index);

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

		case 'r':
			/* TODO */
			help();
			return 0;

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

	ts = ts_setup(tsdevice, 0);
	if (!ts) {
		perror("ts_setup");
		return errno;
	}

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

	r.w = r.h = BLOCK_SIZE;

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
			     "Couldn't create window and renderer: %s", SDL_GetError());
		goto out;
	}

	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlRenderer);




	/* TODO get xres and yres */
	xres = 800; yres = 600;

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
			ts_close(ts);
			exit(1);
		}

		len = sprintf(cal_buffer, "%d %d %d %d %d %d %d %d %d",
			      cal.a[1], cal.a[2], cal.a[0],
			      cal.a[4], cal.a[5], cal.a[3], cal.a[6],
			      xres, yres);
		if (write(cal_fd, cal_buffer, len) == -1) {
			perror("write");
			ts_close(ts);
			exit(1);
		}
		close(cal_fd);
		i = 0;
	} else {
		printf("Calibration failed.\n");
		i = -1;
	}

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
			if (samp_mt[0][i].valid != 1)
				continue;

			r.x = samp_mt[0][i].x;
			r.y = samp_mt[0][i].y;
			r.w = r.h = BLOCK_SIZE;
			SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
			SDL_RenderFillRect(sdlRenderer, &r);

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
		if (samp_mt[0])
			free(samp_mt[0]);

		free(samp_mt);
	}
	return 0;
}
