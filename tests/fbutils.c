/*
 * fbutils.c
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
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>

#include "font.h"

static int con_fd, fb_fd, last_vt = -1;
static struct fb_fix_screeninfo fix;
static struct fb_var_screeninfo var;
static struct fb_cmap cmap;
static char *fbuffer;
static int fb_fd=0;
int xres, yres;

static char *defaultfbdevice = "/dev/fb0";
static char *defaultconsoledevice = "/dev/tty";
static char *fbdevice = NULL;
static char *consoledevice = NULL;

int open_framebuffer(void)
{
        struct vt_stat vts;
        char vtname[128];
        int fd, nr;
	unsigned short col[2];

	if( (fbdevice = getenv("TSLIB_FBDEVICE")) == NULL)
		fbdevice = defaultfbdevice;

	if( (consoledevice = getenv("TSLIB_CONSOLEDEVICE")) == NULL)
		consoledevice = defaultconsoledevice;

	if(strcmp(consoledevice,"none")!=0) {
		sprintf(vtname,"%s%d", consoledevice, 1);
        	fd = open(vtname, O_WRONLY);
        	if (fd < 0) {
        	        perror("open consoledevice");
        	        return -1;
        	}

        	if (ioctl(fd, VT_OPENQRY, &nr) < 0) {
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

        	if (ioctl(con_fd, VT_WAITACTIVE, nr) < 0) {
        	        perror("VT_WAITACTIVE");
        	        close(con_fd);
        	        return -1;
        	}

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
	xres = var.xres;
	yres = var.yres;

	cmap.start = 0;
	cmap.len = 2;
	cmap.red = col;
	cmap.green = col;
	cmap.blue = col;
	cmap.transp = NULL;

	col[0] = 0;
	col[1] = 0xffff;

	if(var.bits_per_pixel==8) {
		if (ioctl(fb_fd, FBIOPUTCMAP, &cmap) < 0) {
			perror("ioctl FBIOPUTCMAP");
			close(fb_fd);
			return -1;
		}
	}

	fbuffer = mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fb_fd, 0);
	if (fbuffer == (char *)-1) {
		perror("mmap framebuffer");
		close(fb_fd);
		return -1;
	}
	memset(fbuffer,0,fix.smem_len);

	return 0;
}

void close_framebuffer(void)
{
	munmap(fbuffer, fix.smem_len);
	close(fb_fd);


	if(strcmp(consoledevice,"none")!=0) {
	
        	if (ioctl(con_fd, KDSETMODE, KD_TEXT) < 0)
        	        perror("KDSETMODE");

        	if (last_vt >= 0)
        	        if (ioctl(con_fd, VT_ACTIVATE, last_vt))
        	                perror("VT_ACTIVATE");

        	close(con_fd);
	}
}

void put_cross(int x, int y, int c)
{
	int off, i, s, e, loc;

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= var.xres) x = var.xres - 1;
	if (y >= var.yres) y = var.yres - 1;

	s = x - 10;
	if(s<0) s=0;
	e = x + 10;
	if(e>var.xres) e= var.xres;
	for(i=s;i<e;i++) {
		loc = (i + var.xoffset)*(var.bits_per_pixel/8)
			+ (y + var.yoffset)*fix.line_length;
		switch(var.bits_per_pixel) {
			case 8:
			default:
				//fbuffer[loc] = c;
				fbuffer[loc] = fbuffer[loc] ? 0 : 1;
				break;
			case 16:
				*((unsigned short *)(fbuffer + loc)) = *((unsigned short *)(fbuffer + loc)) ? 0 : 0xffff;
				break;
			case 24:
			case 32:
				*((unsigned int *)(fbuffer + loc)) = *((unsigned int *)(fbuffer + loc)) ? 0 : 0xffffffff;
				break;
		}
	}
	s = y - 10;
	if(s<0) s=0;
	e = y + 10;
	if(e>var.yres) e = var.yres;
	for(i=s;i<e;i++) {
		loc = (x + var.xoffset)*(var.bits_per_pixel/8)
			+ (i + var.yoffset)*fix.line_length;
		switch(var.bits_per_pixel) {
			case 8:
			default:
				//fbuffer[loc] = c;
				fbuffer[loc] = fbuffer[loc] ? 0 : 1;
				break;
			case 16:
				*((unsigned short *)(fbuffer + loc)) = *((unsigned short *)(fbuffer + loc)) ? 0 : 0xffff;
				break;
			case 24:
			case 32:
				*((unsigned int *)(fbuffer + loc)) = *((unsigned int *)(fbuffer + loc)) ? 0 : 0xffffffff;
				break;
		}
	}
	return;
}

void put_char(int x, int y, int c, int color)
{
	int i,j,bits,loc;

	for(i=0;i<font_vga_8x8.height;i++) {
		bits = font_vga_8x8.data[font_vga_8x8.height*c + i];
		for(j=0;j<8;j++) {
			loc = (x + j + var.xoffset)*(var.bits_per_pixel/8)
				+ (y + i + var.yoffset)*fix.line_length;
			if(loc>=0 && loc<fix.smem_len && ((bits >> (7-j)) & 1)) {
				switch(var.bits_per_pixel) {
					case 8:
					default:
						if(color==0)
							fbuffer[loc] = 0;
						else
							fbuffer[loc] = 1;
						break;
					case 16:
						if(color==0)
							*((unsigned short *)(fbuffer + loc)) = 0;
						else
							*((unsigned short *)(fbuffer + loc)) = 0xffff;
						break;
					case 24:
					case 32:
						if(color==0)
							*((unsigned int *)(fbuffer + loc)) = 0;
						else
							*((unsigned int *)(fbuffer + loc)) = 0xffffffff;
						break;
				}
			}	
		}
	}
}
		


void put_string(int x, int y, char *s, int color)
{
	int i;
	for(i=0;i<strlen(s);i++) {
		put_char( (x + font_vga_8x8.width* (i - strlen(s)/2)), y, s[i], color);
	}
}

void setcolors(int bgcolor, int fgcolor) {
/* No longer implemented
	unsigned short red[2], green[2], blue[2];

	red[0] = ( (bgcolor >> 16) & 0xff ) << 8;
	red[1] = ( (fgcolor >> 16) & 0xff ) << 8;
	green[0] = ( (bgcolor >> 8) & 0xff ) << 8;
	green[1] = ( (fgcolor >> 8) & 0xff ) << 8;
	blue[0] = ( bgcolor & 0xff ) << 8;
	blue[1] = ( fgcolor & 0xff ) << 8;
        cmap.start = 0;
        cmap.len = 2;
        cmap.red = red;
        cmap.green = green;
        cmap.blue = blue;
        cmap.transp = NULL;

	if(var.bits_per_pixel==8) {
        	if (ioctl(fb_fd, FBIOPUTCMAP, &cmap) < 0) {
        	        perror("ioctl FBIOPUTCMAP");
        	        close(fb_fd);
		}
	}
*/
}


