/*
 * fbutils-linux.c
 *
 * Utility routines for framebuffer interaction
 *
 * Copyright 2002 Russell King and Doug Lowder
 *
 * This file is placed under the GPL.  Please see the
 * file COPYING for details.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>

#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/fb.h>

#include "font.h"
#include "fbutils.h"

union multiptr {
	uint8_t *p8;
	uint16_t *p16;
	uint32_t *p32;
};

static int32_t con_fd, last_vt = -1;
static struct fb_fix_screeninfo fix;
static struct fb_var_screeninfo var;
static unsigned char *fbuffer;
static unsigned char **line_addr;
static int32_t fb_fd;
static int32_t bytes_per_pixel;
static uint32_t transp_mask;
static uint32_t colormap[256];
uint32_t xres, yres;
uint32_t xres_orig, yres_orig;
int8_t rotation;
int8_t alternative_cross;

static char *defaultfbdevice = "/dev/fb0";
static char *defaultconsoledevice = "/dev/tty";
static char *fbdevice;
static char *consoledevice;

#define VTNAME_LEN 128

int open_framebuffer(void)
{
	struct vt_stat vts;
	char vtname[VTNAME_LEN];
	int32_t fd, nr;
	uint32_t y, addr;

	if ((fbdevice = getenv("TSLIB_FBDEVICE")) == NULL)
		fbdevice = defaultfbdevice;

	if ((consoledevice = getenv("TSLIB_CONSOLEDEVICE")) == NULL)
		consoledevice = defaultconsoledevice;

	if (strcmp(consoledevice, "none") != 0) {
		if (strlen(consoledevice) >= VTNAME_LEN)
			return -1;

		sprintf(vtname, "%s%d", consoledevice, 1);
		fd = open(vtname, O_WRONLY);
		if (fd < 0) {
			perror("open consoledevice");
			return -1;
		}

		if (ioctl(fd, VT_OPENQRY, &nr) < 0) {
			close(fd);
			perror("ioctl VT_OPENQRY");
			return -1;
		}
		close(fd);

		sprintf(vtname, "%s%d", consoledevice, nr);

		con_fd = open(vtname, O_RDWR | O_NDELAY);
		if (con_fd < 0) {
			perror("open tty");
			return -1;
		}

		if (ioctl(con_fd, VT_GETSTATE, &vts) == 0)
			last_vt = vts.v_active;

		if (ioctl(con_fd, VT_ACTIVATE, nr) < 0) {
			perror("VT_ACTIVATE");
			close(con_fd);
			return -1;
		}

#ifndef TSLIB_NO_VT_WAITACTIVE
		if (ioctl(con_fd, VT_WAITACTIVE, nr) < 0) {
			perror("VT_WAITACTIVE");
			close(con_fd);
			return -1;
		}
#endif

		if (ioctl(con_fd, KDSETMODE, KD_GRAPHICS) < 0) {
			perror("KDSETMODE");
			close(con_fd);
			return -1;
		}

	}

	fb_fd = open(fbdevice, O_RDWR);
	if (fb_fd == -1) {
		perror("open fbdevice");
		return -1;
	}

	if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix) < 0) {
		perror("ioctl FBIOGET_FSCREENINFO");
		close(fb_fd);
		return -1;
	}

	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var) < 0) {
		perror("ioctl FBIOGET_VSCREENINFO");
		close(fb_fd);
		return -1;
	}

	xres_orig = var.xres;
	yres_orig = var.yres;

	if (rotation & 1) {
		/* 1 or 3 */
		y = var.yres;
		yres = var.xres;
		xres = y;
	} else {
		/* 0 or 2 */
		xres = var.xres;
		yres = var.yres;
	}

	fbuffer = mmap(NULL,
		       fix.smem_len,
		       PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED,
		       fb_fd,
		       0);

	if (fbuffer == (unsigned char *)-1) {
		perror("mmap framebuffer");
		close(fb_fd);
		return -1;
	}
	memset(fbuffer, 0, fix.smem_len);

	bytes_per_pixel = (var.bits_per_pixel + 7) / 8;
	transp_mask = ((1 << var.transp.length) - 1) <<
		var.transp.offset; /* transp.length unlikely > 32 */
	line_addr = malloc(sizeof(*line_addr) * var.yres_virtual);
	addr = 0;
	for (y = 0; y < var.yres_virtual; y++, addr += fix.line_length)
		line_addr[y] = fbuffer + addr;

	return 0;
}

void close_framebuffer(void)
{
	memset(fbuffer, 0, fix.smem_len);
	munmap(fbuffer, fix.smem_len);
	close(fb_fd);

	if (strcmp(consoledevice, "none") != 0) {
		if (ioctl(con_fd, KDSETMODE, KD_TEXT) < 0)
			perror("KDSETMODE");

		if (last_vt >= 0)
			if (ioctl(con_fd, VT_ACTIVATE, last_vt))
				perror("VT_ACTIVATE");

		close(con_fd);
	}

	free(line_addr);

	xres = 0;
	yres = 0;
	rotation = 0;
}

void put_cross(int32_t x, int32_t y, uint32_t colidx)
{
	line(x - 10, y, x - 2, y, colidx);
	line(x + 2, y, x + 10, y, colidx);
	line(x, y - 10, x, y - 2, colidx);
	line(x, y + 2, x, y + 10, colidx);

	if (!alternative_cross) {
		line(x - 6, y - 9, x - 9, y - 9, colidx + 1);
		line(x - 9, y - 8, x - 9, y - 6, colidx + 1);
		line(x - 9, y + 6, x - 9, y + 9, colidx + 1);
		line(x - 8, y + 9, x - 6, y + 9, colidx + 1);
		line(x + 6, y + 9, x + 9, y + 9, colidx + 1);
		line(x + 9, y + 8, x + 9, y + 6, colidx + 1);
		line(x + 9, y - 6, x + 9, y - 9, colidx + 1);
		line(x + 8, y - 9, x + 6, y - 9, colidx + 1);
	} else if (alternative_cross == 1) {
		line(x - 7, y - 7, x - 4, y - 4, colidx + 1);
		line(x - 7, y + 7, x - 4, y + 4, colidx + 1);
		line(x + 4, y - 4, x + 7, y - 7, colidx + 1);
		line(x + 4, y + 4, x + 7, y + 7, colidx + 1);
	}
}

static void put_char(int32_t x, int32_t y, int32_t c, int32_t colidx)
{
	int32_t i, j, bits;

	for (i = 0; i < font_vga_8x8.height; i++) {
		bits = font_vga_8x8.data[font_vga_8x8.height * c + i];
		for (j = 0; j < font_vga_8x8.width; j++, bits <<= 1)
			if (bits & 0x80)
				pixel(x + j, y + i, colidx);
	}
}

void put_string(int32_t x, int32_t y, char *s, uint32_t colidx)
{
	int32_t i;

	for (i = 0; *s; i++, x += font_vga_8x8.width, s++)
		put_char (x, y, *s, colidx);
}

void put_string_center(int32_t x, int32_t y, char *s, uint32_t colidx)
{
	size_t sl = strlen(s);

	put_string(x - (sl / 2) * font_vga_8x8.width,
		   y - font_vga_8x8.height / 2, s, colidx);
}

void setcolor(uint32_t colidx, uint32_t value)
{
	uint32_t res;
	uint16_t red, green, blue;
	struct fb_cmap cmap;

	if (colidx > 255) {
#ifdef DEBUG
		fprintf(stderr, "WARNING: color index = %u, must be <256\n",
			colidx);
#endif
		return;
	}

	switch (bytes_per_pixel) {
	default:
	case 1:
		res = colidx;
		red = (value >> 8) & 0xff00;
		green = value & 0xff00;
		blue = (value << 8) & 0xff00;
		cmap.start = colidx;
		cmap.len = 1;
		cmap.red = &red;
		cmap.green = &green;
		cmap.blue = &blue;
		cmap.transp = NULL;

		if (ioctl(fb_fd, FBIOPUTCMAP, &cmap) < 0)
			perror("ioctl FBIOPUTCMAP");
		break;
	case 2:
	case 3:
	case 4:
		red = (value >> 16) & 0xff;
		green = (value >> 8) & 0xff;
		blue = value & 0xff;
		res = ((red >> (8 - var.red.length)) << var.red.offset) |
		      ((green >> (8 - var.green.length)) << var.green.offset) |
		      ((blue >> (8 - var.blue.length)) << var.blue.offset);
	}
	colormap[colidx] = res;
}

static void __pixel_loc(int32_t x, int32_t y, union multiptr *loc)
{
	switch (rotation) {
	case 0:
	default:
		loc->p8 = line_addr[y] + x * bytes_per_pixel;
		break;
	case 1:
		loc->p8 = line_addr[x] + (yres - y - 1) * bytes_per_pixel;
		break;
	case 2:
		loc->p8 = line_addr[yres - y - 1] + (xres - x - 1) * bytes_per_pixel;
		break;
	case 3:
		loc->p8 = line_addr[xres - x - 1] + y * bytes_per_pixel;
		break;
	}
}

static inline void __setpixel(union multiptr loc, uint32_t xormode, uint32_t color)
{
	switch (bytes_per_pixel) {
	case 1:
	default:
		if (xormode)
			*loc.p8 ^= color;
		else
			*loc.p8 = color;
		*loc.p8 |= transp_mask;
		break;
	case 2:
		if (xormode)
			*loc.p16 ^= color;
		else
			*loc.p16 = color;
		*loc.p16 |= transp_mask;
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
		*loc.p8 |= transp_mask;
		break;
	case 4:
		if (xormode)
			*loc.p32 ^= color;
		else
			*loc.p32 = color;
		*loc.p32 |= transp_mask;
		break;
	}
}

void pixel(int32_t x, int32_t y, uint32_t colidx)
{
	uint32_t xormode;
	union multiptr loc;

	if ((x < 0) || ((uint32_t)x >= xres) ||
	    (y < 0) || ((uint32_t)y >= yres))
		return;

	xormode = colidx & XORMODE;
	colidx &= ~XORMODE;

	if (colidx > 255) {
#ifdef DEBUG
		fprintf(stderr, "WARNING: color value = %u, must be <256\n",
			colidx);
#endif
		return;
	}

	__pixel_loc(x, y, &loc);
	__setpixel(loc, xormode, colormap[colidx]);
}

void line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t colidx)
{
	int32_t tmp;
	int32_t dx = x2 - x1;
	int32_t dy = y2 - y1;

	if (abs(dx) < abs(dy)) {
		if (y1 > y2) {
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			dx = -dx; dy = -dy;
		}
		x1 <<= 16;
		/* dy is apriori >0 */
		dx = (dx << 16) / dy;
		while (y1 <= y2) {
			pixel(x1 >> 16, y1, colidx);
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
			pixel(x1, y1 >> 16, colidx);
			y1 += dy;
			x1++;
		}
	}
}

void rect(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t colidx)
{
	line(x1, y1, x2, y1, colidx);
	line(x2, y1+1, x2, y2-1, colidx);
	line(x2, y2, x1, y2, colidx);
	line(x1, y2-1, x1, y1+1, colidx);
}

void fillrect(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t colidx)
{
	int32_t tmp;
	uint32_t xormode;
	union multiptr loc;

	/* Clipping and sanity checking */
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }

	if (x1 < 0)
		x1 = 0;
	if ((uint32_t)x1 >= xres)
		x1 = xres - 1;

	if (x2 < 0)
		x2 = 0;
	if ((uint32_t)x2 >= xres)
		x2 = xres - 1;

	if (y1 < 0)
		y1 = 0;
	if ((uint32_t)y1 >= yres)
		y1 = yres - 1;

	if (y2 < 0)
		y2 = 0;
	if ((uint32_t)y2 >= yres)
		y2 = yres - 1;

	if ((x1 > x2) || (y1 > y2))
		return;

	xormode = colidx & XORMODE;
	colidx &= ~XORMODE;

	if (colidx > 255) {
#ifdef DEBUG
		fprintf(stderr, "WARNING: color value = %u, must be <256\n",
			colidx);
#endif
		return;
	}

	colidx = colormap[colidx];

	for (; y1 <= y2; y1++) {
		for (tmp = x1; tmp <= x2; tmp++) {
			__pixel_loc(tmp, y1, &loc);
			__setpixel(loc, xormode, colidx);
			loc.p8 += bytes_per_pixel;
		}
	}
}
