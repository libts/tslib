/*
 * Copyright (C) 2017 Martin Keppligner <martink@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifndef _SDLUTILS_H
#define _SDLUTILS_H
void draw_line(SDL_Renderer *r, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void draw_crosshair(SDL_Renderer *r, int32_t x, int32_t y);
void getxy(struct tsdev *ts, int *x, int *y);
#endif /* _SDLUTILS_H */
