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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "tslib-private.h"
#include "tslib-filter.h"

#include "cy8mrln.h"

#define NOISE          12
#define SCREEN_WIDTH   319
#define SCREEN_HEIGHT  527
#define H_FIELDS       7
#define V_FIELDS       11

#define DEBUG

#define container_of(ptr, type, member) ({ \
	const typeof( ((type*)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type, member)); })

struct cy8mrln_palmpre_input
{  
	uint16_t n_r;
	uint16_t field[H_FIELDS * V_FIELDS];
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
	uint16_t nulls[H_FIELDS * V_FIELDS];
};

static int scanrate = 60;
static int verbose = 0;
static int wot_threshold = 22;
static int sleepmode = CY8MRLN_ON_STATE;
static int wot_scanrate = WOT_SCANRATE_512HZ;
static int timestamp_mode = 1;

static int 
cy8mrln_palmpre_set_scanrate(struct tsdev* dev, int rate)
{
	if (dev == NULL || ioctl(dev->fd,CY8MRLN_IOCTL_SET_SCANRATE,&rate) < 0)
		goto error;
		
	scanrate = rate;
	return 0;
	
error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set scanrate value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_verbose(struct tsdev* dev, int v)
{
	if (dev == NULL || ioctl(dev->fd,CY8MRLN_IOCTL_SET_VERBOSE_MODE,&v) < 0)
		goto error;
	
	verbose = v;
	return 0;

error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set verbose value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_sleepmode(struct tsdev* dev, int mode)
{
	if (dev == NULL || ioctl(dev->fd,CY8MRLN_IOCTL_SET_SLEEPMODE,&mode) < 0)
		goto error;

	sleepmode = mode;
	return 0;
	
error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set sleepmode value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_wot_scanrate(struct tsdev* dev, int rate)
{
	if (dev == NULL || ioctl(dev->fd,CY8MRLN_IOCTL_SET_WOT_SCANRATE,&rate) < 0)
		goto error;

	wot_scanrate = rate;
	return 0;

error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set scanrate value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_wot_threshold(struct tsdev* dev, int v)
{
	if (dev == NULL) 
		goto error;
	if(v < WOT_THRESHOLD_MIN || v > WOT_THRESHOLD_MAX)
		goto error;
	if(ioctl(dev->fd,CY8MRLN_IOCTL_SET_WOT_THRESHOLD,&v) < 0)
		goto error;
		
	wot_threshold = v;
	return 0;
	
error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set wot treshhold value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_timestamp_mode(struct tsdev* dev, int v)
{
	v = v ? 1 : 0;
	if(dev == NULL || ioctl(dev->fd,CY8MRLN_IOCTL_SET_TIMESTAMP_MODE,&v) < 0)
	     goto error;
	timestamp_mode = v;
	return 0;
	
error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set timestamp value\n");
	return -1;
}

static int
parse_scanrate(struct tslib_module_info *info, char *str, void *data)
{
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long rate = strtoul(str, NULL, 0);

	if(rate == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_scanrate(i->module.dev, scanrate);
}

static int
parse_verbose(struct tslib_module_info *info, char *str, void *data)
{
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long v = strtoul(str, NULL, 0);

	if(v == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_verbose(i->module.dev, scanrate);
}

static int
parse_wot_scanrate(struct tslib_module_info *info, char *str, void *data)
{
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long rate = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_wot_scanrate(i->module.dev, rate);
}

static int
parse_wot_threshold(struct tslib_module_info *info, char *str, void *data)
{
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long threshold = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_wot_threshold(i->module.dev, threshold);
}

static int
parse_sleepmode(struct tslib_module_info *info, char *str, void *data)
{
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long sleep = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_sleepmode(i->module.dev, sleep);
}

static int
parse_timestamp_mode(struct tslib_module_info *info, char *str, void *data)
{
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long sleep = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_sleepmode(i->module.dev, sleep);
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

void
interpolate(uint16_t field[H_FIELDS * V_FIELDS], int i, struct ts_sample *out) {
	float f11, f12, f13, f21, f22, f23, f31, f32, f33;
	int x = SCREEN_WIDTH / H_FIELDS * (H_FIELDS - (i % H_FIELDS));
	int y = SCREEN_HEIGHT / (V_FIELDS - 1) * (i / H_FIELDS);
	static int dx = SCREEN_WIDTH / H_FIELDS;
	static int dy = SCREEN_HEIGHT / V_FIELDS;
	
	/* caluculate corrections for top, bottom, left and right fields */
	f12 = (i < (H_FIELDS + 1)) ? 0.0 : 0.5 * field[i - H_FIELDS] / field[i];
	f32 = (i > (H_FIELDS * (V_FIELDS - 2)))
	      ? 0.0 : 0.5 * field[i + H_FIELDS] / field[i];
	f21 = (i % H_FIELDS == (H_FIELDS - 1))
	      ? 0.0 : 0.5 * field[i + 1] / field[i];
	f23 = (i % H_FIELDS == 0) ? 0.0 : 0.5 * field[i - 1] / field[i];

	/*
	   correct values for the edges, shift the mesuarment point by half a 
	   field diminsion to the outside
	*/
	if (i == 0) {
		x = x + dx / 2;
		f21 = f21 * 2.0;
		y = y - dy / 2;
		f32 = f32 * 2.0;
	} else if (i == (H_FIELDS - 1)) {
		x = x - dx / 2;
		f23 = f23 * 2.0;
		y = y - dy / 2;
		f32 = f32 * 2.0;
	} else if (i % H_FIELDS == (H_FIELDS - 1)) {
		x = x - dx / 2;
		f23 = f23 * 2.0;
	} else if (i % H_FIELDS == 0) {
		x = x + dx / 2;
		f21 = f21 * 2.0;
	} else if (i < H_FIELDS) {
		y = y - dy / 2;
		f32 = f32 * 2.0;
	} else if (i == (H_FIELDS * (V_FIELDS - 2))) {
		x = x + dx / 2;
		f21 = f21 * 2.0;
		y = y + dy / 2;
		f12 = f12 * 2.0;
	} else if (i == (H_FIELDS * (V_FIELDS - 1) - 1)) {
		x = x - dx / 2;
		f23 = f23 * 2.0;
		y = y + dy / 2;
		f12 = f12 * 2.0;
	} else if (i > (H_FIELDS * (V_FIELDS - 2))) {
		y = y + dy / 2;
		f12 = f12 * 2.0;
	}

	/* caluclate corrections for the corners */
/*
	f11 = (i % H_FIELDS == (H_FIELDS - 1) || i < (H_FIELDS + 1))
	      ? 0.0 : 0.4 * field[i - H_FIELDS + 1] / field[i];
	f13 = (i % H_FIELDS == 0 || i < (H_FIELDS + 1))
	      ? 0.0 : 0.4 * field[i - H_FIELDS - 1] / field[i];
	f31 = (i % H_FIELDS == (H_FIELDS - 1)
	       || i > (H_FIELDS * (V_FIELDS - 1) - 1))
	      ? 0.0 : 0.4 * field[i + H_FIELDS + 1] / field[i];
	f33 = (i % H_FIELDS == 0 || i > (H_FIELDS * (V_FIELDS - 1) - 1))
	      ? 0.0 : 0.4 * field[i + H_FIELDS - 1] / field[i];
*/

	out->x = x // + (f13 + f33 - f11 - f31) * dx /* use corners too?*/
		 + (f23 - f21) * dx
//	         + (f21 == 0.0) ? ((f23 * 2 + (dx / 2)) * dx) : (f23 * dx)
//	         - (f23 == 0.0) ? ((f21 * 2 + (dx / 2)) * dx) : (f21 * dx)
	         - (dx / 2);
	out->y = y // + (f31 + f33 - f11 - f13) * dy /* use corners too?*/
	         + (f32 - f12) * dy + (dy / 2);

	out->pressure = field[i];
#ifdef DEBUG
	printf("RAW--------------------------->f21: %f (%d), f23: %f (%d), f12: %f (%d), f32: %f (%d), \n", 
	       f21, field[i - 1], f23, field[i + 1], f12,
	       field[i - H_FIELDS], f32, field[i + H_FIELDS]);
#endif /*DEBUG*/
}

static int
cy8mrln_palmpre_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = info->dev;
	struct cy8mrln_palmpre_input *cy8mrln_evt;
	struct tslib_cy8mrln_palmpre *cy8mrln_info;
	int max_index = 0, max_value = 0, i = 0;
	uint16_t tmp_value;
	int ret;
	
	cy8mrln_info = container_of(info, struct tslib_cy8mrln_palmpre, module);
	
	cy8mrln_evt = alloca(sizeof(*cy8mrln_evt) * nr);
	ret = read(ts->fd, cy8mrln_evt, sizeof(*cy8mrln_evt) * nr);
	if (ret > 0) {
		max_index = 0;
		max_value = 0;
		while(ret >= (int)sizeof(*cy8mrln_evt)) {
                        for (i = 0; i < (H_FIELDS * V_FIELDS); i++) {
				/* auto calibrate zero values */
				if (cy8mrln_evt->field[i] > cy8mrln_info->nulls[i])
					cy8mrln_info->nulls[i] = cy8mrln_evt->field[i];

				tmp_value = abs(cy8mrln_info->nulls[i] - cy8mrln_evt->field[i]);

				/* check for the maximum value */
				if (tmp_value > max_value) {
					max_value = tmp_value;
					max_index = i;
				}

				cy8mrln_evt->field[i] = tmp_value;
			}
			/* only caluclate events that are not noise */
			if (max_value > NOISE) {
				interpolate(cy8mrln_evt->field, max_index, samp);
#ifdef DEBUG
				fprintf(stderr,"RAW---------------------------> %d %d %d\n",
					samp->x, samp->y, samp->pressure);
#endif /*DEBUG*/
			}

			gettimeofday(&samp->tv,NULL);
			samp++;
			cy8mrln_evt++;
			ret -= sizeof(*cy8mrln_evt);
		}
	} else {
		return -1;
	}

	ret = nr;
	return ret;
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

	// FIXME why do we read here one packet from fd?
	read(dev->fd, &input, sizeof(input));

	memcpy(info->nulls, input.field, H_FIELDS * V_FIELDS * sizeof(uint16_t));

	return &(info->module);
}
#ifndef TSLIB_STATIC_CY8MRLN_MODULE
	TSLIB_MODULE_INIT(cy8mrln_palmpre_mod_init);
#endif
