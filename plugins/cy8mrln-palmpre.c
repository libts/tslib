/*
 *  tslib/plugins/cy8mrln-palmpre.c
 *
 *  Copyright (C) 2010 Frederik Sdun <frederik.sdun@googlemail.com>
 *		       Thomas Zimmermann <ml@vdm-design.de>
 *
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Pluging for the cy8mrln touchscreen with the Firmware used on the Palm Pre
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/spi/cy8mrln.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "tslib-private.h"
#include "tslib-filter.h"

struct cy8mrln_palmpre_input
{  
	uint16_t n_r;
	uint16_t values[7][11];
	uint16_t ffff;		  //seperator? always ff
	uint8_t seq_nr1;		//incremented if seq_nr0 = scanrate
	uint16_t seq_nr2;	       //incremeted if seq_nr1 = 255
	uint8_t unknown[4]; 
	uint8_t seq_nr0;		//incremented. 0- scanrate
	uint8_t null;		   //\0
}__attribute__((packed));

struct tslib_cy8mrln_palmpre 
{
	struct tslib_module_info module;
	uint16_t nulls[7][11];
};

static int scanrate = 60;
static int verbose = 1;
static int wot_threshold = 22;
static int sleepmode = CY8MRLN_ON_STATE;
static int wot_scanrate = WOT_SCANRATE_512HZ;
static int timestamp_mode = 1;

static int 
cy8mrln_palmpre_set_scanrate(struct tsdev* dev, int rate)
{
	if(ioctl(dev->fd,CY8MRLN_IOCTL_SET_SCANRATE,&rate) < 0)
	     return -1;
	scanrate = rate;
	return 0;
}

static int 
cy8mrln_palmpre_set_verbose(struct tsdev* dev, int v)
{
	if(ioctl(dev->fd,CY8MRLN_IOCTL_SET_VERBOSE_MODE,&v) < 0)
	     return -1;
	verbose = v;
	return 0;
}

static int 
cy8mrln_palmpre_set_sleepmode(struct tsdev* dev, int mode)
{
	if(ioctl(dev->fd,CY8MRLN_IOCTL_SET_SLEEPMODE,&mode) < 0)
	     return -1;
	sleepmode = mode;
	return 0;
}

static int 
cy8mrln_palmpre_set_wot_scanrate(struct tsdev* dev, int rate)
{
	if(ioctl(dev->fd,CY8MRLN_IOCTL_SET_WOT_SCANRATE,&rate) < 0)
	     return -1;
	wot_scanrate = rate;
	return 0;
}

static int 
cy8mrln_palmpre_set_wot_threshold(struct tsdev* dev, int v)
{
	if(v < WOT_THRESHOLD_MIN || v > WOT_THRESHOLD_MAX)
	     return -1;
	if(ioctl(dev->fd,CY8MRLN_IOCTL_SET_WOT_THRESHOLD,&v) < 0)
	     return -1;
	wot_threshold = v;
	return 0;
}

static int 
cy8mrln_palmpre_set_timestamp_mode(struct tsdev* dev, int v)
{
	v = v ? 1 : 0;
	if(ioctl(dev->fd,CY8MRLN_IOCTL_SET_TIMESTAMP_MODE,&v) < 0)
	     return -1;
	timestamp_mode = v;
	return 0;
}

static int
parse_scanrate(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long rate = strtoul(str, NULL, 0);

	if(rate == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_scanrate(i->module.dev, scanrate);
}

static int
parse_verbose(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long v = strtoul(str, NULL, 0);

	if(v == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_verbose(i->module.dev, scanrate);
}

static int
parse_wot_scanrate(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long rate = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_wot_scanrate(i->module.dev, rate);
}

static int
parse_wot_threshold(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long threshold = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_wot_threshold(i->module.dev, threshold);
}

static int
parse_sleepmode(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long sleep = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_sleepmode(i->module.dev, sleep);
	return 0;
}

static int
parse_timestamp_mode(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long sleep = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_sleepmode(i->module.dev, sleep);
	return 0;
}

static const struct tslib_vars raw_vars[] =
{
	{ "scanrate", NULL, parse_scanrate},
	{ "verbose", NULL, parse_verbose},
	{ "wot_scanrate", NULL, parse_wot_scanrate},
	{ "wot_threshold", NULL, parse_wot_threshold},
	{ "sleepmode", NULL, parse_sleepmode},
	{ "timestamp_mode", NULL, parse_timestamp_mode}
};

#define NR_VARS (sizeof(raw_vars) / sizeof(raw_vars[0]))

static int
cy8mrln_palmpre_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	return 0;
}

static int 
cy8mrln_palmpre_fini(struct tslib_module_info *inf)
{
	return 0;
}


static const struct tslib_ops cy8mrln_palmpre_ops = 
{
	.read = cy8mrln_palmpre_read,
	.fini = cy8mrln_palmpre_fini,
};

TSAPI struct tslib_module_info *cy8mrln_palmpre_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_cy8mrln_palmpre *info;
        struct cy8mrln_palmpre_input input;
	
	info = malloc(sizeof(struct tslib_cy8mrln_palmpre));
	if(info == NULL)
	     return NULL;
	info->module.ops = &cy8mrln_palmpre_ops;

        cy8mrln_palmpre_set_verbose(dev,verbose);
        cy8mrln_palmpre_set_scanrate(dev,scanrate);
        cy8mrln_palmpre_set_timestamp_mode(dev,timestamp_mode);
        cy8mrln_palmpre_set_sleepmode(dev,sleepmode);
        cy8mrln_palmpre_set_wot_scanrate(dev,wot_scanrate);
        cy8mrln_palmpre_set_wot_threshold(dev,wot_threshold);

	if (tslib_parse_vars(&info->module, raw_vars, NR_VARS, params)) {
		free(info);
		return NULL;
	}

        read(dev->fd, &input, sizeof(input));

        memcpy(info->nulls, input.values, 7*11*sizeof(uint16_t));

	return &(info->module);
}
#ifndef TSLIB_STATIC_CY8MRLN_MODULE
	TSLIB_MODULE_INIT(cy8mrln_palmpre_mod_init);
#endif
