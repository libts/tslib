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

#include <stdint.h>

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

