/*
 * fbutils-drm.c
 *
 * Utility routines for DRM/KMS framebuffer interaction
 * Alternative to fbutils-linux.c using libdrm instead of fbdev.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#include "font.h"
#include "fbutils.h"

union multiptr {
	uint8_t *p8;
	uint16_t *p16;
	uint32_t *p32;
};

static int drm_fd = -1;
static drmModeConnector *connector;
static drmModeEncoder *encoder;
static drmModeCrtc *orig_crtc;
static drmModeModeInfo mode;

static uint32_t crtc_id;
static uint32_t connector_id;
static uint32_t fb_id;

static uint32_t drm_handle;
static uint32_t drm_pitch;
static uint32_t drm_size;
static unsigned char *fbuffer;
static unsigned char **line_addr;

static int32_t bytes_per_pixel;
static uint32_t colormap[256];

uint32_t xres, yres;
uint32_t xres_orig, yres_orig;
int8_t rotation;
int8_t alternative_cross;

static char *defaultdrmdevice = "/dev/dri/card0";

static drmModeConnector *find_connector(int fd, drmModeRes *res)
{
	int i;

	for (i = 0; i < res->count_connectors; i++) {
		drmModeConnector *conn = drmModeGetConnector(fd, res->connectors[i]);

		if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0)
			return conn;

		drmModeFreeConnector(conn);
	}

	return NULL;
}

static drmModeEncoder *find_encoder(int fd, drmModeConnector *conn)
{
	if (conn->encoder_id)
		return drmModeGetEncoder(fd, conn->encoder_id);

	return NULL;
}

static uint32_t find_crtc(int fd, drmModeRes *res, drmModeConnector *conn)
{
	drmModeEncoder *enc;
	int i, j;

	/* Try the currently attached encoder */
	if (conn->encoder_id) {
		enc = drmModeGetEncoder(fd, conn->encoder_id);
		if (enc) {
			if (enc->crtc_id) {
				uint32_t id = enc->crtc_id;

				drmModeFreeEncoder(enc);
				return id;
			}
			drmModeFreeEncoder(enc);
		}
	}

	/* No CRTC currently set - find a suitable default */
	for (i = 0; i < conn->count_encoders; i++) {
		enc = drmModeGetEncoder(fd, conn->encoders[i]);
		if (!enc)
			continue;

		for (j = 0; j < res->count_crtcs; j++) {
			if (enc->possible_crtcs & (1u << j)) {
				uint32_t id = res->crtcs[j];

				drmModeFreeEncoder(enc);
				return id;
			}
		}
		drmModeFreeEncoder(enc);
	}

	return 0;
}

int open_framebuffer(void)
{
	char *drmdevice;
	drmModeRes *res;
	struct drm_mode_create_dumb creq;
	struct drm_mode_map_dumb mreq;
	uint32_t y, addr;

	if ((drmdevice = getenv("TSLIB_DRMDEVICE")) == NULL)
		drmdevice = defaultdrmdevice;

	drm_fd = open(drmdevice, O_RDWR | O_CLOEXEC);
	if (drm_fd < 0) {
		perror("open drm device");
		return -1;
	}

	res = drmModeGetResources(drm_fd);
	if (!res) {
		perror("drmModeGetResources");
		close(drm_fd);
		drm_fd = -1;
		return -1;
	}

	connector = find_connector(drm_fd, res);
	if (!connector) {
		fprintf(stderr, "No connected DRM connector found\n");
		drmModeFreeResources(res);
		close(drm_fd);
		drm_fd = -1;
		return -1;
	}
	connector_id = connector->connector_id;

	/* Use the first (preferred) mode */
	mode = connector->modes[0];

	crtc_id = find_crtc(drm_fd, res, connector);
	if (!crtc_id) {
		fprintf(stderr, "No suitable CRTC found for DRM connector\n");
		drmModeFreeConnector(connector);
		drmModeFreeResources(res);
		close(drm_fd);
		drm_fd = -1;
		return -1;
	}

	/* Save original CRTC state for restoration */
	orig_crtc = drmModeGetCrtc(drm_fd, crtc_id);

	encoder = find_encoder(drm_fd, connector);

	drmModeFreeResources(res);

	/* Create a dumb buffer */
	memset(&creq, 0, sizeof(creq));
	creq.width = mode.hdisplay;
	creq.height = mode.vdisplay;
	creq.bpp = 32;

	if (drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0) {
		perror("DRM_IOCTL_MODE_CREATE_DUMB");
		goto err_cleanup;
	}

	drm_handle = creq.handle;
	drm_pitch = creq.pitch;
	drm_size = creq.size;
	bytes_per_pixel = 4;

	/* Create framebuffer object */
	if (drmModeAddFB(drm_fd, mode.hdisplay, mode.vdisplay,
			 24, 32, drm_pitch, drm_handle, &fb_id)) {
		perror("drmModeAddFB");
		goto err_destroy;
	}

	/* Map the dumb buffer */
	memset(&mreq, 0, sizeof(mreq));
	mreq.handle = drm_handle;

	if (drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0) {
		perror("DRM_IOCTL_MODE_MAP_DUMB");
		goto err_rmfb;
	}

	fbuffer = mmap(NULL, drm_size, PROT_READ | PROT_WRITE,
		       MAP_SHARED, drm_fd, mreq.offset);
	if (fbuffer == MAP_FAILED) {
		perror("mmap drm dumb buffer");
		goto err_rmfb;
	}
	memset(fbuffer, 0, drm_size);

	/* Set the mode on the CRTC */
	if (drmModeSetCrtc(drm_fd, crtc_id, fb_id, 0, 0,
			   &connector_id, 1, &mode)) {
		perror("drmModeSetCrtc");
		goto err_unmap;
	}

	xres_orig = mode.hdisplay;
	yres_orig = mode.vdisplay;

	if (rotation & 1) {
		xres = mode.vdisplay;
		yres = mode.hdisplay;
	} else {
		xres = mode.hdisplay;
		yres = mode.vdisplay;
	}

	line_addr = malloc(sizeof(*line_addr) * mode.vdisplay);
	if (!line_addr)
		goto err_unmap;

	addr = 0;
	for (y = 0; y < mode.vdisplay; y++, addr += drm_pitch)
		line_addr[y] = fbuffer + addr;

	return 0;

err_unmap:
	munmap(fbuffer, drm_size);
err_rmfb:
	drmModeRmFB(drm_fd, fb_id);
err_destroy:
	{
		struct drm_mode_destroy_dumb dreq = { .handle = drm_handle };
		drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	}
err_cleanup:
	if (encoder)
		drmModeFreeEncoder(encoder);
	drmModeFreeConnector(connector);
	if (orig_crtc)
		drmModeFreeCrtc(orig_crtc);
	close(drm_fd);
	drm_fd = -1;
	return -1;
}

void close_framebuffer(void)
{
	struct drm_mode_destroy_dumb dreq;

	if (drm_fd < 0)
		return;

	memset(fbuffer, 0, drm_size);

	/* Restore original CRTC */
	if (orig_crtc) {
		drmModeSetCrtc(drm_fd, orig_crtc->crtc_id,
			       orig_crtc->buffer_id,
			       orig_crtc->x, orig_crtc->y,
			       &connector_id, 1, &orig_crtc->mode);
		drmModeFreeCrtc(orig_crtc);
	}

	munmap(fbuffer, drm_size);
	drmModeRmFB(drm_fd, fb_id);

	dreq.handle = drm_handle;
	drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);

	if (encoder)
		drmModeFreeEncoder(encoder);
	drmModeFreeConnector(connector);
	close(drm_fd);

	free(line_addr);

	drm_fd = -1;
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
		put_char(x, y, *s, colidx);
}

void put_string_center(int32_t x, int32_t y, char *s, uint32_t colidx)
{
	size_t sl = strlen(s);

	put_string(x - (sl / 2) * font_vga_8x8.width,
		   y - font_vga_8x8.height / 2, s, colidx);
}

void setcolor(uint32_t colidx, uint32_t value)
{
	uint32_t red, green, blue;

	if (colidx > 255) {
#ifdef DEBUG
		fprintf(stderr, "WARNING: color index = %u, must be <256\n",
			colidx);
#endif
		return;
	}

	/* DRM dumb buffer is always XRGB8888 (32bpp) */
	red = (value >> 16) & 0xff;
	green = (value >> 8) & 0xff;
	blue = value & 0xff;
	colormap[colidx] = (red << 16) | (green << 8) | blue;
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
	/* DRM dumb buffer is always 32bpp */
	if (xormode)
		*loc.p32 ^= color;
	else
		*loc.p32 = color;
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
		}
	}
}
