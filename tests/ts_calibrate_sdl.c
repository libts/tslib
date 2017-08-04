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
#include "sdlutils.h"

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

static int sort_by_x(const void* a, const void *b)
{
	return (((struct ts_sample *)a)->x - ((struct ts_sample *)b)->x);
}

static int sort_by_y(const void* a, const void *b)
{
	return (((struct ts_sample *)a)->y - ((struct ts_sample *)b)->y);
}

void getxy(struct tsdev *ts, int *x, int *y)
{
#define MAX_SAMPLES 128
	struct ts_sample samp[MAX_SAMPLES];
	int index, middle;

	do {
		if (ts_read_raw(ts, &samp[0], 1) < 0) {
			perror("ts_read");
			SDL_Quit();
		}
		
	} while (samp[0].pressure == 0);

	/* Now collect up to MAX_SAMPLES touches into the samp array. */
	index = 0;
	do {
		if (index < MAX_SAMPLES-1)
			index++;
		if (ts_read_raw(ts, &samp[index], 1) < 0) {
			perror("ts_read");
			SDL_Quit();
		}
	} while (samp[index].pressure > 0);
	printf("Took %d samples...\n",index);

	/*
	 * At this point, we have samples in indices zero to (index-1)
	 * which means that we have (index) number of samples.  We want
	 * to calculate the median of the samples so that wild outliers
	 * don't skew the result.  First off, let's assume that arrays
	 * are one-based instead of zero-based.  If this were the case
	 * and index was odd, we would need sample number ((index+1)/2)
	 * of a sorted array; if index was even, we would need the
	 * average of sample number (index/2) and sample number
	 * ((index/2)+1).  To turn this into something useful for the
	 * real world, we just need to subtract one off of the sample
	 * numbers.  So for when index is odd, we need sample number
	 * (((index+1)/2)-1).  Due to integer division truncation, we
	 * can simplify this to just (index/2).  When index is even, we
	 * need the average of sample number ((index/2)-1) and sample
	 * number (index/2).  Calculate (index/2) now and we'll handle
	 * the even odd stuff after we sort.
	 */
	middle = index/2;
	if (x) {
		qsort(samp, index, sizeof(struct ts_sample), sort_by_x);
		if (index & 1)
			*x = samp[middle].x;
		else
			*x = (samp[middle-1].x + samp[middle].x) / 2;
	}
	if (y) {
		qsort(samp, index, sizeof(struct ts_sample), sort_by_y);
		if (index & 1)
			*y = samp[middle].y;
		else
			*y = (samp[middle-1].y + samp[middle].y) / 2;
	}
}

static void get_sample(struct tsdev *ts, calibration *cal,
		       int index, int x, int y, char *name,
		       SDL_Renderer *r)
{
	static int last_x = -1, last_y;
	SDL_Event ev;

	if (last_x != -1) {

#define NR_STEPS 20
		int dx = ((x - last_x) << 16) / NR_STEPS;
		int dy = ((y - last_y) << 16) / NR_STEPS;
		int i;

		last_x <<= 16;
		last_y <<= 16;
		for (i = 0; i < NR_STEPS; i++) {
			SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
			SDL_RenderClear(r);
			draw_crosshair(r, last_x >> 16, last_y >> 16);
			SDL_RenderPresent(r);

			last_x += dx;
			last_y += dy;
		}
	}

	SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
	SDL_RenderClear(r);
	draw_crosshair(r, x, y);
	SDL_RenderPresent(r);

	getxy(ts, &cal->x[index], &cal->y[index]);

	SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
	SDL_RenderClear(r);
	draw_crosshair(r, x, y);
	SDL_RenderPresent(r);

	last_x = cal->xfb[index] = x;
	last_y = cal->yfb[index] = y;

	printf("%s : X = %4d Y = %4d\n", name, cal->x[index], cal->y[index]);

	SDL_PollEvent(&ev);
	switch (ev.type) {
		case SDL_KEYDOWN:
		case SDL_QUIT:
			SDL_ShowCursor(SDL_ENABLE);
			SDL_Quit();
	}

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
	const char *tsdevice = NULL;
	short verbose = 0;
	int ret;
	int i;
	SDL_Window *sdlWindow;
	SDL_Renderer *sdlRenderer;
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
	SDL_DisplayMode currentDisplay;

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
			/* TODO add rotation */
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

	/* TODO add support for selecting the display */
	if (SDL_GetNumVideoDisplays() > 1) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "More than one display found");
		goto out;
	}

	ret = SDL_GetCurrentDisplayMode(0, &currentDisplay);
	if (ret) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "Could not get display mode for display #1: %s",
			     SDL_GetError());
		goto out;
	}

	xres = currentDisplay.w;
	yres = currentDisplay.h;

	printf("xres = %d, yres = %d\n", xres, yres);

	/* Clear the buffer */
	clearbuf(ts);

	get_sample(ts, &cal, 0, 50,        50,        "Top left", sdlRenderer);
	clearbuf(ts);
	get_sample(ts, &cal, 1, xres - 50, 50,        "Top right", sdlRenderer);
	clearbuf(ts);
	get_sample(ts, &cal, 2, xres - 50, yres - 50, "Bot right", sdlRenderer);
	clearbuf(ts);
	get_sample(ts, &cal, 3, 50,        yres - 50, "Bot left", sdlRenderer);
	clearbuf(ts);
	get_sample(ts, &cal, 4, xres / 2,  yres / 2,  "Center", sdlRenderer);

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

out:
	if (ts)
		ts_close(ts);

	return 0;
}
