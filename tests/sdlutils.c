/*
 * Copyright (C) 2017 Martin Keppligner <martink@posteo.de>
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

#include <stdint.h>

#include <tslib.h>

void draw_line(SDL_Renderer *r, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	int32_t tmp;
	int32_t dx = x2 - x1;
	int32_t dy = y2 - y1;

	if (abs(dx) < abs(dy)) {
		if (y1 > y2) {
			tmp = x1;
			x1 = x2;
			x2 = tmp;

			tmp = y1;
			y1 = y2;
			y2 = tmp;

			dx = -dx;
			dy = -dy;
		}

		x1 <<= 16;

		dx = (dx << 16) / dy;
		while (y1 <= y2) {
			SDL_RenderDrawPoint(r, x1 >> 16, y1);
			y1 += dx;
			y1++;
		}
	} else {
		if (x1 > x2) {
			tmp = x1;
			x1 = x2;
			x2 = tmp;

			tmp = y1;
			y1 = y2;
			y2 = tmp;

			dx = -dx;
			dy = -dy;
		}

		y1 <<= 16;

		dy = dx ? (dy << 16) : 0;
		while (x1 <= x2) {
			SDL_RenderDrawPoint(r, x1, y1 >> 16);
			y1 += dy;
			x1++;
		}
	}
}

void draw_crosshair(SDL_Renderer *r, int32_t x, int32_t y)
{
	SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
	draw_line(r, x - 10, y, x - 2, y);
	draw_line(r, x + 2, y, x + 10, y);
	draw_line(r, x, y - 10, x, y - 2);
	draw_line(r, x, y + 2, x, y + 10);

	SDL_SetRenderDrawColor(r, 0xff, 0xe0, 0x80, 255);
	draw_line(r, x - 6, y - 9, x - 9, y - 9);
	draw_line(r, x - 9, y - 8, x - 9, y - 6);
	draw_line(r, x - 9, y + 6, x - 9, y + 9);
	draw_line(r, x - 8, y + 9, x - 6, y + 9);
	draw_line(r, x + 6, y + 9, x + 9, y + 9);
	draw_line(r, x + 9, y + 8, x + 9, y + 6);
	draw_line(r, x + 9, y - 6, x + 9, y - 9);
	draw_line(r, x + 8, y - 9, x + 6, y - 9);
}

static int sort_by_x(const void *a, const void *b)
{
	return (((struct ts_sample *)a)->x - ((struct ts_sample *)b)->x);
}

static int sort_by_y(const void *a, const void *b)
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
	printf("Took %d samples...\n", index);

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
