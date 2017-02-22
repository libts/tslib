/*
 * fbutils-bsd.c
 *
 * Utility routines for framebuffer interaction
 *
 * Copyright 2002 Russell King and Doug Lowder
 *
 * This file is placed under the GPL.  Please see the
 * file COPYING for details.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <sys/consio.h>
#include <sys/fbio.h>

#include "font.h"
#include "fbutils.h"

union multiptr {
	unsigned char *p8;
	unsigned short *p16;
	unsigned long *p32;
};

static int fbsize;
static unsigned char *fbuffer;
static unsigned char **line_addr;
static int fb_fd=0;
static int bytes_per_pixel;
static unsigned colormap [256];
uint32_t xres, yres;

static char *defaultfbdevice = "/dev/fb0";
static char *fbdevice = NULL;

int open_framebuffer(void)
{
	int y;
	unsigned addr;
	struct fbtype fb;
	int line_length;

	if ((fbdevice = getenv ("TSLIB_FBDEVICE")) == NULL)
		fbdevice = defaultfbdevice;

	fb_fd = open(fbdevice, O_RDWR);
	if (fb_fd == -1) {
		perror("open fbdevice");
		return -1;
	}


	if (ioctl(fb_fd, FBIOGTYPE, &fb) != 0) {
		perror("ioctl(FBIOGTYPE)");
		return -1;
	}

	if (ioctl(fb_fd, FBIO_GETLINEWIDTH, &line_length) != 0) {
		perror("ioctl(FBIO_GETLINEWIDTH)");
		return -1;
	}

	xres = fb.fb_width;
	yres = fb.fb_height;

	int pagemask = getpagesize() - 1;
	fbsize = ((int) line_length*yres + pagemask) & ~pagemask;

	fbuffer = (unsigned char *)mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if (fbuffer == (unsigned char *)-1) {
		perror("mmap framebuffer");
		close(fb_fd);
		return -1;
	}
	memset(fbuffer,0, fbsize);

	bytes_per_pixel = (fb.fb_depth + 7) / 8;
	line_addr = malloc (sizeof (line_addr) * fb.fb_height);
	addr = 0;
	for (y = 0; y < fb.fb_height; y++, addr += line_length)
		line_addr [y] = fbuffer + addr;

	return 0;
}

void close_framebuffer(void)
{
	munmap(fbuffer, fbsize);
	close(fb_fd);

        free (line_addr);
}

void put_cross(int x, int y, unsigned colidx)
{
	fprintf(stderr, "%d %d, %d\n", x, y, colidx);

	line (x - 10, y, x - 2, y, colidx);
	line (x + 2, y, x + 10, y, colidx);
	line (x, y - 10, x, y - 2, colidx);
	line (x, y + 2, x, y + 10, colidx);

#if 1
	line (x - 6, y - 9, x - 9, y - 9, colidx + 1);
	line (x - 9, y - 8, x - 9, y - 6, colidx + 1);
	line (x - 9, y + 6, x - 9, y + 9, colidx + 1);
	line (x - 8, y + 9, x - 6, y + 9, colidx + 1);
	line (x + 6, y + 9, x + 9, y + 9, colidx + 1);
	line (x + 9, y + 8, x + 9, y + 6, colidx + 1);
	line (x + 9, y - 6, x + 9, y - 9, colidx + 1);
	line (x + 8, y - 9, x + 6, y - 9, colidx + 1);
#else
	line (x - 7, y - 7, x - 4, y - 4, colidx + 1);
	line (x - 7, y + 7, x - 4, y + 4, colidx + 1);
	line (x + 4, y - 4, x + 7, y - 7, colidx + 1);
	line (x + 4, y + 4, x + 7, y + 7, colidx + 1);
#endif
}

void put_char(int x, int y, int c, int colidx)
{
	int i,j,bits;

	for (i = 0; i < font_vga_8x8.height; i++) {
		bits = font_vga_8x8.data [font_vga_8x8.height * c + i];
		for (j = 0; j < font_vga_8x8.width; j++, bits <<= 1)
			if (bits & 0x80)
				pixel (x + j, y + i, colidx);
	}
}

void put_string(int x, int y, char *s, unsigned colidx)
{
	int i;
	for (i = 0; *s; i++, x += font_vga_8x8.width, s++)
		put_char (x, y, *s, colidx);
}

void put_string_center(int x, int y, char *s, unsigned colidx)
{
	size_t sl = strlen (s);
        put_string (x - (sl / 2) * font_vga_8x8.width,
                    y - font_vga_8x8.height / 2, s, colidx);
}

void setcolor(unsigned colidx, unsigned value)
{
	unsigned res;
	unsigned char red, green, blue;

#ifdef DEBUG
	if (colidx > 255) {
		fprintf (stderr, "WARNING: color index = %u, must be <256\n",
			 colidx);
		return;
	}
#endif

	red = (value >> 16) & 0xff;
	green = (value >> 8) & 0xff;
	blue = value & 0xff;

	switch (bytes_per_pixel) {
	case 1:
		res = value;
		break;
	case 2:
		res = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
		break;
	case 3:
	case 4:
	default:
		res = value;

	}
        colormap [colidx] = res;
}

static inline void __setpixel (union multiptr loc, unsigned xormode, unsigned color)
{
	switch(bytes_per_pixel) {
	case 1:
	default:
		if (xormode)
			*loc.p8 ^= color;
		else
			*loc.p8 = color;
		break;
	case 2:
		if (xormode)
			*loc.p16 ^= color;
		else
			*loc.p16 = color;
		break;
	case 3:
		if (xormode) {
			*loc.p8++ ^= (color >> 16) & 0xff;
			*loc.p8++ ^= (color >> 8) & 0xff;
			*loc.p8 ^= color & 0xff;
		} else {
			*loc.p8++ = (color >> 16) & 0xff;
			*loc.p8++ = (color >> 8) & 0xff;
			*loc.p8 = color & 0xff;
		}
		break;
	case 4:
		if (xormode)
			*loc.p32 ^= color;
		else
			*loc.p32 = color;
		break;
	}
}

void pixel (int x, int y, unsigned colidx)
{
	unsigned xormode;
	union multiptr loc;

	if ((x < 0) || ((uint32_t)x >= xres) ||
	    (y < 0) || ((uint32_t)y >= yres))
		return;

	xormode = colidx & XORMODE;
	colidx &= ~XORMODE;

#ifdef DEBUG
	if (colidx > 255) {
		fprintf (stderr, "WARNING: color value = %u, must be <256\n",
			 colidx);
		return;
	}
#endif

	loc.p8 = line_addr [y] + x * bytes_per_pixel;
	__setpixel (loc, xormode, colormap [colidx]);
}

void line (int x1, int y1, int x2, int y2, unsigned colidx)
{
	int tmp;
	int dx = x2 - x1;
	int dy = y2 - y1;

	if (abs (dx) < abs (dy)) {
		if (y1 > y2) {
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			dx = -dx; dy = -dy;
		}
		x1 <<= 16;
		/* dy is apriori >0 */
		dx = (dx << 16) / dy;
		while (y1 <= y2) {
			pixel (x1 >> 16, y1, colidx);
			x1 += dx;
			y1++;
		}
	} else {
		if (x1 > x2) {
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			dx = -dx; dy = -dy;
		}
		y1 <<= 16;
		dy = dx ? (dy << 16) / dx : 0;
		while (x1 <= x2) {
			pixel (x1, y1 >> 16, colidx);
			y1 += dy;
			x1++;
		}
	}
}

void rect (int x1, int y1, int x2, int y2, unsigned colidx)
{
	line (x1, y1, x2, y1, colidx);
	line (x2, y1+1, x2, y2-1, colidx);
	line (x2, y2, x1, y2, colidx);
	line (x1, y2-1, x1, y1+1, colidx);
}

void fillrect (int x1, int y1, int x2, int y2, unsigned colidx)
{
	int tmp;
	unsigned xormode;
	union multiptr loc;

	/* Clipping and sanity checking */
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
	if (x1 < 0) x1 = 0; if ((uint32_t)x1 >= xres) x1 = xres - 1;
	if (x2 < 0) x2 = 0; if ((uint32_t)x2 >= xres) x2 = xres - 1;
	if (y1 < 0) y1 = 0; if ((uint32_t)y1 >= yres) y1 = yres - 1;
	if (y2 < 0) y2 = 0; if ((uint32_t)y2 >= yres) y2 = yres - 1;

	if ((x1 > x2) || (y1 > y2))
		return;

	xormode = colidx & XORMODE;
	colidx &= ~XORMODE;

#ifdef DEBUG
	if (colidx > 255) {
		fprintf (stderr, "WARNING: color value = %u, must be <256\n",
			 colidx);
		return;
	}
#endif

	colidx = colormap [colidx];

	for (; y1 <= y2; y1++) {
		loc.p8 = line_addr [y1] + x1 * bytes_per_pixel;
		for (tmp = x1; tmp <= x2; tmp++) {
			__setpixel (loc, xormode, colidx);
			loc.p8 += bytes_per_pixel;
		}
	}
}
