/*
 * fbutils.h
 *
 * Headers for utility routines for framebuffer interaction
 *
 * Copyright 2002 Russell King and Doug Lowder
 *
 * This file is placed under the GPL.  Please see the
 * file COPYING for details.
 *
 */

extern int xres, yres;

int open_framebuffer(void);
void close_framebuffer(void);
void put_cross(int x, int y, int c);
void put_string(int x, int y, char *s, int color);
void setcolors(int bgcolor, int fgcolor);
