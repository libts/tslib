/*
 *  font.h -- `Soft' font definitions
 *
 *  Copyright (C) 1995 Geert Uytterhoeven
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _VIDEO_FONT_H
#define _VIDEO_FONT_H

struct fbcon_font_desc {
	int idx;
	char *name;
	int width, height;
	unsigned char *data;
	int pref;
};

#define VGA8x8_IDX	0
#define VGA8x16_IDX	1
#define PEARL8x8_IDX	2
#define VGA6x11_IDX	3
#define SUN8x16_IDX	4
#define SUN12x22_IDX	5
#define ACORN8x8_IDX	6

extern struct fbcon_font_desc	font_vga_8x8,
				font_vga_8x16;

/* Max. length for the name of a predefined font */
#define MAX_FONT_NAME	32

#endif /* _VIDEO_FONT_H */
