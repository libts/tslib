/*
 *  tslib/plugins/cy8mrln-palmpre.c
 *
 *  Copyright (C) 2010 Frederik Sdun <frederik.sdun@googlemail.com>
 *                     Thomas Zimmermann <ml@vdm-design.de>
 *                     Simon Busch <morphis@gravedo.de>
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * Plugin for the cy8mrln touchscreen with the firmware used on the Palm Pre (Plus).
 *
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
#include <linux/spi/cy8mrln.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "config.h"
#include "tslib-private.h"
#include "tslib-filter.h"

#define SCREEN_WIDTH   319
#define SCREEN_HEIGHT  527
#define H_FIELDS       7
#define V_FIELDS       11
#define DEFAULT_SCANRATE 60
#define DEFAULT_VERBOSE 0
#define DEFAULT_WOT_THRESHOLD 22
#define DEFAULT_SLEEPMODE CY8MRLN_ON_STATE
#define DEFAULT_WOT_SCANRATE WOT_SCANRATE_512HZ
#define DEFAULT_TIMESTAMP_MODE 1
#define DEFAULT_TS_PRESSURE 255
#define DEFAULT_NOISE 25

#define container_of(ptr, type, member) ({ \
	const typeof( ((type*)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type, member)); })

struct cy8mrln_palmpre_input
{  
	uint16_t	n_r;
	uint16_t	field[H_FIELDS * V_FIELDS];
	uint16_t	ffff;			/* always 0xffff */
	uint8_t		seq_nr1;		/* incremented if seq_nr0 == scanrate */
	uint16_t	seq_nr2;		/* incremeted if seq_nr1 == 255 */
	uint8_t		unknown[4]; 
	uint8_t		seq_nr0;		/* incremented [0:scanrate] */
	uint8_t		null;		   /* NULL byte */
}__attribute__((packed));

struct tslib_cy8mrln_palmpre 
{
	struct tslib_module_info module;
	uint16_t nulls[H_FIELDS * V_FIELDS];
        int scanrate;
        int verbose;
        int wot_threshold;
        int sleepmode;
        int wot_scanrate;
        int timestamp_mode;
        int ts_pressure;
        int noise;
};


static int 
cy8mrln_palmpre_set_scanrate(struct tslib_cy8mrln_palmpre* info, int rate)
{
	if (info == NULL || info->module.dev == NULL || ioctl(info->module.dev->fd,CY8MRLN_IOCTL_SET_SCANRATE,&rate) < 0)
		goto error;
		
	info->scanrate = rate;
	return 0;
	
error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set scanrate value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_verbose(struct tslib_cy8mrln_palmpre* info, int v)
{
	if (info == NULL || info->module.dev == NULL || ioctl(info->module.dev->fd,CY8MRLN_IOCTL_SET_VERBOSE_MODE,&v) < 0)
		goto error;
	
	info->verbose = v;
	return 0;

error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set verbose value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_sleepmode(struct tslib_cy8mrln_palmpre* info, int mode)
{
	if (info == NULL || info->module.dev == NULL || ioctl(info->module.dev->fd,CY8MRLN_IOCTL_SET_SLEEPMODE,&mode) < 0)
		goto error;

	info->sleepmode = mode;
	return 0;
	
error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set sleepmode value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_wot_scanrate(struct tslib_cy8mrln_palmpre* info, int rate)
{
	if (info == NULL || info->module.dev == NULL || ioctl(info->module.dev->fd,CY8MRLN_IOCTL_SET_WOT_SCANRATE,&rate) < 0)
		goto error;

	info->wot_scanrate = rate;
	return 0;

error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set scanrate value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_wot_threshold(struct tslib_cy8mrln_palmpre* info, int v)
{
	if (info == NULL || info->module.dev == NULL) 
		goto error;
	if(v < WOT_THRESHOLD_MIN || v > WOT_THRESHOLD_MAX)
		goto error;
	if(ioctl(info->module.dev->fd,CY8MRLN_IOCTL_SET_WOT_THRESHOLD,&v) < 0)
		goto error;
		
	info->wot_threshold = v;
	return 0;
	
error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set wot treshhold value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_timestamp_mode(struct tslib_cy8mrln_palmpre* info, int v)
{
	v = v ? 1 : 0;
	if(info == NULL || info->module.dev == NULL || ioctl(info->module.dev->fd,CY8MRLN_IOCTL_SET_TIMESTAMP_MODE,&v) < 0)
	     goto error;
	info->timestamp_mode = v;
	return 0;
	
error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set timestamp value\n");
	return -1;
}

static int 
cy8mrln_palmpre_set_noise (struct tslib_cy8mrln_palmpre* info, int n)
{
        if (info == NULL) {
                printf("TSLIB: cy8mrln_palmpre: ERROR: could not set noise value\n");
                return -1;
        }
        info->noise = n;
        return 0;
}

static int
cy8mrln_palmpre_set_ts_pressure(struct tslib_cy8mrln_palmpre* info, int p)
{
        if (info == NULL) {
                printf("TSLIB: cy8mrln_palmpre: ERROR: could not set ts_pressure value\n");
                return -1;
        }
        info->ts_pressure = p;
        return 0;
}

static int
parse_scanrate(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	unsigned long rate = strtoul(str, NULL, 0);

	if(rate == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_scanrate(i, rate);
}

static int
parse_verbose(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	unsigned long v = strtoul(str, NULL, 0);

	if(v == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_verbose(i, v);
}

static int
parse_wot_scanrate(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	unsigned long rate = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_wot_scanrate(i, rate);
}

static int
parse_wot_threshold(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	unsigned long threshold = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_wot_threshold(i, threshold);
}

static int
parse_sleepmode(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	unsigned long sleep = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_sleepmode(i, sleep);
}

static int
parse_timestamp_mode(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long sleep = strtoul(str, NULL, 0);

	if(sleep == ULONG_MAX && errno == ERANGE)
		return -1;


	return cy8mrln_palmpre_set_sleepmode(i, sleep);
}
static int
parse_noise(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
        unsigned long noise = strtoul (str, NULL, 0);

	if(noise == ULONG_MAX && errno == ERANGE)
		return -1;


        return cy8mrln_palmpre_set_noise (i, noise);
}

static int
parse_ts_pressure(struct tslib_module_info *info, char *str, void *data)
{
        (void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
        unsigned long tp = strtoul (str, NULL, 0);

	if(tp == ULONG_MAX && errno == ERANGE)
		return -1;

        return cy8mrln_palmpre_set_ts_pressure (i, tp);
}

static const struct tslib_vars raw_vars[] =
{
	{ "scanrate", NULL, parse_scanrate},
	{ "verbose", NULL, parse_verbose},
	{ "wot_scanrate", NULL, parse_wot_scanrate},
	{ "wot_threshold", NULL, parse_wot_threshold},
	{ "sleepmode", NULL, parse_sleepmode},
	{ "timestamp_mode", NULL, parse_timestamp_mode},
        { "noise", NULL, parse_noise},
        { "ts_pressure", NULL, parse_ts_pressure}
};

#define NR_VARS (sizeof(raw_vars) / sizeof(raw_vars[0]))

void
interpolate(uint16_t field[H_FIELDS * V_FIELDS], int i, struct ts_sample *out) {
	float f12, f21, f22, f23, f32; //f33, f31, f13, f11
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

	/* correct values for the edges, shift the mesuarment point by half a 
	 * field diminsion to the outside */
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

#ifdef DEBUG
	printf("RAW---------------------------> f22: %f (%d) f21: %f (%d), f23: %f (%d), f12: %f (%d), f32: %f (%d), \n", 
	       f22, field[i],
	       f21, field[i - 1], f23, field[i + 1], f12,
	       field[i - H_FIELDS], f32, field[i + H_FIELDS]);
#endif /*DEBUG*/
}

static int
cy8mrln_palmpre_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = info->dev;
        //We can only read one input struct at once
	struct cy8mrln_palmpre_input cy8mrln_evt;
	struct tslib_cy8mrln_palmpre *cy8mrln_info;
	int max_index = 0, max_value = 0, i = 0;
	uint16_t tmp_value;
	int ret, valid_samples = 0;
	struct ts_sample *p = samp;
	
	/* initalize all samples with proper values */
        memset(p, '\0', nr * sizeof (*p));
	
	cy8mrln_info = container_of(info, struct tslib_cy8mrln_palmpre, module);
	
	ret = read(ts->fd, &cy8mrln_evt, sizeof(cy8mrln_evt));
	if (ret > 0) {
		max_index = 0;
		max_value = 0;
                for (i = 0; i < (H_FIELDS * V_FIELDS); i++) {
                        /* auto calibrate zero values */
                        if (cy8mrln_evt.field[i] > cy8mrln_info->nulls[i])
                                cy8mrln_info->nulls[i] = cy8mrln_evt.field[i];

                        tmp_value = abs(cy8mrln_info->nulls[i] - cy8mrln_evt.field[i]);

                        /* check for the maximum value */
                        if (tmp_value > max_value) {
                                max_value = tmp_value;
                                max_index = i;
                        }

                        cy8mrln_evt.field[i] = tmp_value;
                }
                /* only caluclate events that are not noise */
                if (max_value > cy8mrln_info->noise) {
                        interpolate(cy8mrln_evt.field, max_index, samp);
                        samp->pressure = cy8mrln_info -> ts_pressure;
#ifdef DEBUG
                        fprintf(stderr,"RAW---------------------------> %d %d %d\n",
                                samp->x, samp->y, samp->pressure);
#endif /*DEBUG*/
                        gettimeofday(&samp->tv,NULL);
                        samp++;
                        valid_samples++;
                }
	}

	return valid_samples;
}

static int 
cy8mrln_palmpre_fini(struct tslib_module_info *info)
{
        (void)info;
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
	int ret = 0;
	
	info = malloc(sizeof(struct tslib_cy8mrln_palmpre));
	if(info == NULL)
	     return NULL;
	info->module.ops = &cy8mrln_palmpre_ops;

	cy8mrln_palmpre_set_verbose(info, DEFAULT_VERBOSE);
	cy8mrln_palmpre_set_scanrate(info, DEFAULT_SCANRATE);
	cy8mrln_palmpre_set_timestamp_mode(info, DEFAULT_TIMESTAMP_MODE);
	cy8mrln_palmpre_set_sleepmode(info, DEFAULT_SLEEPMODE);
	cy8mrln_palmpre_set_wot_scanrate(info, DEFAULT_WOT_SCANRATE);
	cy8mrln_palmpre_set_wot_threshold(info, DEFAULT_WOT_THRESHOLD);
        cy8mrln_palmpre_set_noise(info, DEFAULT_NOISE);
        cy8mrln_palmpre_set_ts_pressure(info, DEFAULT_TS_PRESSURE);

	if (tslib_parse_vars(&info->module, raw_vars, NR_VARS, params)) {
		free(info);
		return NULL;
	}

	/* We need the intial values the touchscreen repots with no touch input for
	 * later use */
	do {
		ret = read(dev->fd, &input, sizeof(input));
	}
	while (ret <= 0);
	memcpy(info->nulls, input.field, H_FIELDS * V_FIELDS * sizeof(uint16_t));

	return &(info->module);
}
#ifndef TSLIB_STATIC_CY8MRLN_MODULE
	TSLIB_MODULE_INIT(cy8mrln_palmpre_mod_init);
#endif
