/*
 * tslib/plugins/cy8mrln-palmpre.c
 *
 * Copyright (C) 2010-2011 Frederik Sdun <frederik.sdun@googlemail.com>
 *                         Thomas Zimmermann <ml@vdm-design.de>
 *                         Simon Busch <morphis@gravedo.de>
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * Plugin for the cy8mrln touchscreen of the Palm Pre/Pre Plus/Pre 2 devices.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
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
#define DEFAULT_GESTURE_HEIGHT 1
#define DEFAULT_NOISE 25
#define DEFAULT_PRESSURE 60
#define DEFAULT_SENSOR_DELTA_X (SCREEN_WIDTH / H_FIELDS)
#define DEFAULT_SENSOR_DELTA_Y (SCREEN_HEIGHT / (V_FIELDS - DEFAULT_GESTURE_HEIGHT))
#define DEFAULT_SENSOR_OFFSET_X (DEFAULT_SENSOR_DELTA_X / 2)
#define DEFAULT_SENSOR_OFFSET_Y (DEFAULT_SENSOR_DELTA_Y / 2)

#define MIN_VALUE 700
#define MAX_VALUE 1500
#define ASLEEP_SCANRATE 5
#define DISCARD_FRAMES 5

#define field_nr(x, y) (y * H_FIELDS + (H_FIELDS - x) - 1)

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
	struct tslib_module_info	module;
	uint16_t			references[H_FIELDS * V_FIELDS];
	int				scanrate;
	int				verbose;
	int				wot_threshold;
	int				sleepmode;
	int				wot_scanrate;
	int				timestamp_mode;
	int				gesture_height;
	int				noise;
	int				pressure;
	int				sensor_offset_x;
	int				sensor_offset_y;
	int				sensor_delta_x;
	int				sensor_delta_y;
	int				last_n_valid_samples;
	struct ts_sample*		last_valid_samples;
	int 			discard_frames;
	int 			old_scanrate;
};

static int cy8mrln_palmpre_set_scanrate (struct tslib_cy8mrln_palmpre* info, int rate);
static int cy8mrln_palmpre_set_verbose (struct tslib_cy8mrln_palmpre* info, int v);
static int cy8mrln_palmpre_set_sleepmode (struct tslib_cy8mrln_palmpre* info, int mode);
static int cy8mrln_palmpre_set_wot_scanrate (struct tslib_cy8mrln_palmpre* info, int rate);
static int cy8mrln_palmpre_set_wot_threshold (struct tslib_cy8mrln_palmpre* info, int v);
static int cy8mrln_palmpre_set_timestamp_mode (struct tslib_cy8mrln_palmpre* info, int v);
static int cy8mrln_palmpre_set_gesture_height (struct tslib_cy8mrln_palmpre* info, int h);
static int cy8mrln_palmpre_set_noise (struct tslib_cy8mrln_palmpre* info, int n);
static int cy8mrln_palmpre_set_pressure (struct tslib_cy8mrln_palmpre* info, int p);
static int cy8mrln_palmpre_set_sensor_offset_x (struct tslib_cy8mrln_palmpre* info, int n);
static int cy8mrln_palmpre_set_sensor_offset_y (struct tslib_cy8mrln_palmpre* info, int n);
static int cy8mrln_palmpre_set_sensor_delta_x (struct tslib_cy8mrln_palmpre* info, int n);
static int cy8mrln_palmpre_set_sensor_delta_y (struct tslib_cy8mrln_palmpre* info, int n);
static int parse_scanrate (struct tslib_module_info *info, char *str, void *data);
static int parse_verbose (struct tslib_module_info *info, char *str, void *data);
static int parse_wot_scanrate (struct tslib_module_info *info, char *str, void *data);
static int parse_wot_threshold (struct tslib_module_info *info, char *str, void *data);
static int parse_sleepmode (struct tslib_module_info *info, char *str, void *data);
static int parse_timestamp_mode (struct tslib_module_info *info, char *str, void *data);
static int parse_gesture_height (struct tslib_module_info *info, char *str, void *data);
static int parse_noise (struct tslib_module_info *info, char *str, void *data);
static int parse_pressure (struct tslib_module_info *info, char *str, void *data);
static int parse_sensor_offset_x(struct tslib_module_info *info, char *str, void *data);
static int parse_sensor_offset_y(struct tslib_module_info *info, char *str, void *data);
static int parse_sensor_delta_x(struct tslib_module_info *info, char *str, void *data);
static int parse_sensor_delta_y(struct tslib_module_info *info, char *str, void *data);
static int cy8mrln_palmpre_update_references (uint16_t references[H_FIELDS * V_FIELDS], uint16_t field[H_FIELDS * V_FIELDS]);
static void cy8mrln_palmpre_interpolate (struct tslib_cy8mrln_palmpre* info, uint16_t field[H_FIELDS * V_FIELDS], int x, int y, struct ts_sample *out);
static int cy8mrln_palmpre_fini (struct tslib_module_info *info);
static int cy8mrln_palmpre_read (struct tslib_module_info *info, struct ts_sample *samp, int nr);
TSAPI struct tslib_module_info *cy8mrln_palmpre_mod_init (struct tsdev *dev, const char *params);

static int cy8mrln_palmpre_set_scanrate(struct tslib_cy8mrln_palmpre* info, int rate)
{
	if (info == NULL || info->module.dev == NULL || ioctl(info->module.dev->fd,CY8MRLN_IOCTL_SET_SCANRATE,&rate) < 0)
		goto error;
		
	info->scanrate = rate;
	return 0;
	
error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set scanrate value\n");
	return -1;
}

static int cy8mrln_palmpre_set_verbose(struct tslib_cy8mrln_palmpre* info, int v)
{
	if (info == NULL || info->module.dev == NULL || ioctl(info->module.dev->fd,CY8MRLN_IOCTL_SET_VERBOSE_MODE,&v) < 0)
		goto error;
	
	info->verbose = v;
	return 0;

error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set verbose value\n");
	return -1;
}

static int cy8mrln_palmpre_set_sleepmode(struct tslib_cy8mrln_palmpre* info, int mode)
{
	if (info == NULL || info->module.dev == NULL || ioctl(info->module.dev->fd,CY8MRLN_IOCTL_SET_SLEEPMODE,&mode) < 0)
		goto error;

	info->sleepmode = mode;
	return 0;
	
error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set sleepmode value\n");
	return -1;
}

static int cy8mrln_palmpre_set_wot_scanrate(struct tslib_cy8mrln_palmpre* info, int rate)
{
	if (info == NULL || info->module.dev == NULL || ioctl(info->module.dev->fd,CY8MRLN_IOCTL_SET_WOT_SCANRATE,&rate) < 0)
		goto error;

	info->wot_scanrate = rate;
	return 0;

error:
	printf("TSLIB: cy8mrln_palmpre: ERROR: could not set scanrate value\n");
	return -1;
}

static int cy8mrln_palmpre_set_wot_threshold(struct tslib_cy8mrln_palmpre* info, int v)
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

static int cy8mrln_palmpre_set_timestamp_mode(struct tslib_cy8mrln_palmpre* info, int v)
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

static int cy8mrln_palmpre_set_gesture_height (struct tslib_cy8mrln_palmpre* info, int h)
{
	if (info == NULL) {
		printf("TSLIB: cy8mrln_palmpre: ERROR: could not set gesture_height value\n");
		return -1;
	}

	info->gesture_height = h;

	return 0;
}

static int cy8mrln_palmpre_set_noise (struct tslib_cy8mrln_palmpre* info, int n)
{
	if (info == NULL) {
		printf("TSLIB: cy8mrln_palmpre: ERROR: could not set noise value\n");
		return -1;
	}

	info->noise = n;

	return 0;
}

static int cy8mrln_palmpre_set_pressure (struct tslib_cy8mrln_palmpre* info, int p)
{
	if (info == NULL) {
		printf("TSLIB: cy8mrln_palmpre: ERROR: could not set default_pressure value\n");
		return -1;
	}

	info->pressure = p;

	return 0;
}

static int cy8mrln_palmpre_set_sensor_offset_x (struct tslib_cy8mrln_palmpre* info, int n)
{
	if (info == NULL)
		return -1;

#ifdef DEBUG
	printf("sensor_offset_x: %i\n", n);
#endif
	
	info->sensor_offset_x = n;
	return 0;
}

static int cy8mrln_palmpre_set_sensor_offset_y (struct tslib_cy8mrln_palmpre* info, int n)
{
	if (info == NULL)
		return -1;

#ifdef DEBUG 
	printf("sensor_offset_y: %i\n", n);
#endif
	
	info->sensor_offset_y = n;
	return 0;
}

static int cy8mrln_palmpre_set_sensor_delta_x (struct tslib_cy8mrln_palmpre* info, int n)
{
	if (info == NULL)
		return -1;

#ifdef DEBUG
	printf("sensor_delta_x: %i\n", n);
#endif

	info->sensor_delta_x = n;
	return 0;
}

static int cy8mrln_palmpre_set_sensor_delta_y (struct tslib_cy8mrln_palmpre* info, int n)
{
	if (info == NULL)
		return -1;

#ifdef DEBUG
	printf("sensor_delta_y: %i\n", n);
#endif

	info->sensor_delta_y = n;
	return 0;
}

static int parse_scanrate(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	unsigned long rate = strtoul(str, NULL, 0);

	if(rate == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_scanrate(i, rate);
}

static int parse_verbose(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	unsigned long v = strtoul(str, NULL, 0);

	if(v == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_verbose(i, v);
}

static int parse_wot_scanrate(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	unsigned long rate = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_wot_scanrate(i, rate);
}

static int parse_wot_threshold(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	unsigned long threshold = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_wot_threshold(i, threshold);
}

static int parse_sleepmode(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	unsigned long sleep = strtoul(str, NULL, 0);

	return cy8mrln_palmpre_set_sleepmode(i, sleep);
}

static int parse_timestamp_mode(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long sleep = strtoul(str, NULL, 0);

	if(sleep == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_sleepmode(i, sleep);
}

static int parse_gesture_height(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long gesture_height = strtoul (str, NULL, 0);

	if(gesture_height == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_gesture_height (i, gesture_height);
}

static int parse_noise(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long noise = strtoul (str, NULL, 0);

	if(noise == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_noise (i, noise);
}

static int parse_pressure(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long pressure = strtoul (str, NULL, 0);

	if(pressure == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_pressure (i, pressure);
}

static int parse_sensor_offset_x(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long x = strtoul (str, NULL, 0);

	if(x == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_sensor_offset_x (i, x);
}

static int parse_sensor_offset_y(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long y = strtoul (str, NULL, 0);

	if(y == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_sensor_offset_y (i, y);
}

static int parse_sensor_delta_x(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long x = strtoul (str, NULL, 0);

	if(x == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_sensor_delta_x (i, x);
}

static int parse_sensor_delta_y(struct tslib_module_info *info, char *str, void *data)
{
	(void)data;
	struct tslib_cy8mrln_palmpre *i = (struct tslib_cy8mrln_palmpre*) info;
	unsigned long y = strtoul (str, NULL, 0);

	if(y == ULONG_MAX && errno == ERANGE)
		return -1;

	return cy8mrln_palmpre_set_sensor_delta_y (i, y);
}

#define NR_VARS (sizeof(cy8mrln_palmpre_vars) / sizeof(cy8mrln_palmpre_vars[0]))

/*
*     y1
* x1 (xy) x3
*     y3
*/

static void cy8mrln_palmpre_interpolate(struct tslib_cy8mrln_palmpre* info, uint16_t field[H_FIELDS * V_FIELDS], int x, int y, struct ts_sample *out)
{
	float fx, fy;
	int tmpxy, tmpx1, tmpx3, tmpy1, tmpy3;
	int posx = info->sensor_delta_x * x + info->sensor_offset_x;
	int posy = info->sensor_delta_y * y + info->sensor_offset_y;

	tmpxy = field[field_nr(x, y)];

	if (x == (H_FIELDS - 1)) {
		tmpx3 = info->pressure - tmpxy;
	} else {
		tmpx3 = field[field_nr(x, y) - 1];
	}
	if (x == 0)
		tmpx1 = info->pressure - tmpxy;
	else
		tmpx1 = field[field_nr(x, y) + 1];

	if (y == (V_FIELDS - 1)) {
		tmpy3 = info->pressure - tmpxy;
	} else {
		tmpy3 = field[field_nr(x, y) + H_FIELDS];
	}
	if (y == 0)
		tmpy1 = info->pressure - tmpxy;
	else
		tmpy1 = field[field_nr(x, y) - H_FIELDS];

	fx = (float)(tmpx3 - tmpx1) / ((float)tmpxy * 1.5);
	fy = (float)(tmpy3 - tmpy1) / ((float)tmpxy * 1.5);

	out->x = posx + fx * info->sensor_delta_x;
	out->y = posy + fy * info->sensor_delta_y;

#ifdef DEBUG
	fprintf (stderr, "fx: %f, fy: %f\n", fx, fy);
#endif /*DEBUG*/
}

static int cy8mrln_palmpre_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = info->dev;
	//We can only read one input struct at once
	struct cy8mrln_palmpre_input cy8mrln_evt;
	struct tslib_cy8mrln_palmpre *cy8mrln_info;
	int max_x = 0, max_y = 0, max_value = 0, x, y;
	uint16_t tmp_value;
	int ret, valid_samples = 0;
	struct ts_sample *p = samp;
	
	/* initalize all samples with proper values */
	memset(p, '\0', nr * sizeof (*p));
	
	cy8mrln_info = container_of(info, struct tslib_cy8mrln_palmpre, module);
	
	ret = read(ts->fd, &cy8mrln_evt, sizeof(cy8mrln_evt));
	if (ret > 0) {
		if (cy8mrln_palmpre_update_references (cy8mrln_info->references, cy8mrln_evt.field)) {
			if (cy8mrln_info->discard_frames == 0) {
				//backup current scanrate
				cy8mrln_info->old_scanrate = cy8mrln_info->scanrate;
				cy8mrln_palmpre_set_scanrate (cy8mrln_info, ASLEEP_SCANRATE);
				cy8mrln_info->discard_frames = DISCARD_FRAMES;
#ifdef DEBUG
				fprintf (stderr, "go to sleep\n");
#endif
			}

			return 0;
		}

		//reset scanrate after waking up
		if (cy8mrln_info->discard_frames == DISCARD_FRAMES) {
			cy8mrln_palmpre_set_scanrate (cy8mrln_info, cy8mrln_info->old_scanrate);
#ifdef DEBUG
			fprintf (stderr, "woke up\n");
#endif
		}

		if (cy8mrln_info->discard_frames) {
#ifdef DEBUG
			fprintf (stderr, "cy8mrln_palmpre: discarded frames %i\n", cy8mrln_info->discard_frames);
			for (y = 0; y < V_FIELDS; y ++) {
				for (x = 0; x < H_FIELDS; x++) {
					fprintf (stderr, "%3i", cy8mrln_evt.field[field_nr(x, y)]);
				}
				fprintf (stderr, "\n");
			}
#endif
			cy8mrln_info->discard_frames --;
			//discard frame
			return 0;
		}

		max_x = 0;
		max_y = 0;
		max_value = 0;
		for (y = 0; y < V_FIELDS; y ++) {
			for (x = 0; x < H_FIELDS; x++) {
				tmp_value = cy8mrln_evt.field[field_nr(x, y)];

				/* check for the maximum value */
				if (tmp_value > max_value) {
					max_value = tmp_value;
					max_x = x;
					max_y = y;
				}
			}
		}

		/* only caluclate events that are not noise */
		if (max_value > cy8mrln_info->noise) {
			cy8mrln_palmpre_interpolate(cy8mrln_info, cy8mrln_evt.field, max_x, max_y, &samp[valid_samples]);
			samp->pressure = max_value;
			gettimeofday(&samp->tv,NULL);
			valid_samples++;
			if (cy8mrln_info->last_valid_samples == NULL) {
				cy8mrln_info->last_valid_samples = malloc (sizeof (struct ts_sample) * valid_samples);
			} else if (cy8mrln_info->last_n_valid_samples != valid_samples) {
				cy8mrln_info->last_valid_samples = realloc (cy8mrln_info->last_valid_samples, sizeof (struct ts_sample) * valid_samples);
			}
			memcpy (cy8mrln_info->last_valid_samples, samp, sizeof (struct ts_sample) * valid_samples);
			cy8mrln_info->last_n_valid_samples = valid_samples;
		} else {
			//return last samples with pressure = 0 to show a mouse up
			if (cy8mrln_info->last_valid_samples != NULL) {
				valid_samples = cy8mrln_info->last_n_valid_samples;
				memcpy (samp, cy8mrln_info->last_valid_samples, sizeof (struct ts_sample) * valid_samples);
				for (x = 0; x < valid_samples; x++) {
					samp[x].pressure = 0;
				}
				cy8mrln_info->last_n_valid_samples = 0;
				free (cy8mrln_info->last_valid_samples);
				cy8mrln_info->last_valid_samples = NULL;
#ifdef DEBUG
				fprintf (stderr, "cy8mrln_palmpre: Returning %i old values with 0 pressure\n", valid_samples);
#endif
			}
		}
	} else {
		return -1;
	}

	return valid_samples;
}

static int cy8mrln_palmpre_update_references(uint16_t references[H_FIELDS * V_FIELDS], uint16_t field[H_FIELDS * V_FIELDS])
{
	int x, y;

	for (y = 0; y < V_FIELDS; y ++) {
		for (x = 0; x < H_FIELDS; x++) {
			if (field[y * H_FIELDS + x] < MIN_VALUE || field[y * H_FIELDS + x] > MAX_VALUE) {
#ifdef DEBUG
				fprintf(stderr, "Discrading frame with %i at[%i/%i]\n", field[y * H_FIELDS + x], x, y);
#endif
				return 1;
			}

			if (field[y * H_FIELDS + x] > references[y * H_FIELDS + x]) {
				references [y * H_FIELDS + x] = field [y * H_FIELDS + x];
				field [y * H_FIELDS + x] = 0;
			} else {
				field [y * H_FIELDS + x] = references [y * H_FIELDS + x] - field [y * H_FIELDS + x];
			}
		}
	}

	return 0;
}

static int cy8mrln_palmpre_fini(struct tslib_module_info *info)
{
	struct tslib_cy8mrln_palmpre *i = container_of(info, struct tslib_cy8mrln_palmpre, module);
	if (i->last_valid_samples != NULL) {
		free (i->last_valid_samples);
	}
	free (i);
#ifdef DEBUG
	fprintf (stderr, "finishing cy8mrln_palmpre");
#endif
	return 0;
}

static const struct tslib_vars cy8mrln_palmpre_vars[] =
{
	{ "scanrate",			NULL, parse_scanrate},
	{ "verbose",			NULL, parse_verbose},
	{ "wot_scanrate",		NULL, parse_wot_scanrate},
	{ "wot_threshold",		NULL, parse_wot_threshold},
	{ "sleepmode",			NULL, parse_sleepmode},
	{ "timestamp_mode",		NULL, parse_timestamp_mode},
	{ "gesture_height",		NULL, parse_gesture_height},
	{ "noise",			NULL, parse_noise},
	{ "pressure",			NULL, parse_pressure},
	{ "sensor_offset_x",		NULL, parse_sensor_offset_x},
	{ "sensor_offset_y",		NULL, parse_sensor_offset_y},
	{ "sensor_delta_x",		NULL, parse_sensor_delta_x},
	{ "sensor_delta_y",		NULL, parse_sensor_delta_y},
};

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
        //required to set the default valuse
        info->module.dev = dev;
	info->last_valid_samples = NULL;
	info->last_n_valid_samples = 0;

	cy8mrln_palmpre_set_verbose(info, DEFAULT_VERBOSE);
	cy8mrln_palmpre_set_scanrate(info, DEFAULT_SCANRATE);
	cy8mrln_palmpre_set_timestamp_mode(info, DEFAULT_TIMESTAMP_MODE);
	cy8mrln_palmpre_set_sleepmode(info, DEFAULT_SLEEPMODE);
	cy8mrln_palmpre_set_wot_scanrate(info, DEFAULT_WOT_SCANRATE);
	cy8mrln_palmpre_set_wot_threshold(info, DEFAULT_WOT_THRESHOLD);
	cy8mrln_palmpre_set_gesture_height(info, DEFAULT_GESTURE_HEIGHT);
	cy8mrln_palmpre_set_noise(info, DEFAULT_NOISE);
	cy8mrln_palmpre_set_pressure(info, DEFAULT_PRESSURE);
	cy8mrln_palmpre_set_sensor_offset_x (info, DEFAULT_SENSOR_OFFSET_X);
	cy8mrln_palmpre_set_sensor_offset_y (info, DEFAULT_SENSOR_OFFSET_Y);
	cy8mrln_palmpre_set_sensor_delta_x (info, DEFAULT_SENSOR_DELTA_X);
	cy8mrln_palmpre_set_sensor_delta_y (info, DEFAULT_SENSOR_DELTA_Y);

	info->discard_frames = 0;


	if (tslib_parse_vars(&info->module, cy8mrln_palmpre_vars, NR_VARS, params)) {
		free(info);
		return NULL;
	}

	/* We need the intial values the touchscreen repots with no touch input for
	 * later use */
	do {
		ret = read(dev->fd, &input, sizeof(input));
	}
	while (ret <= 0);
	memcpy(info->references, input.field, H_FIELDS * V_FIELDS * sizeof(uint16_t));

	return &(info->module);
}
#ifndef TSLIB_STATIC_CY8MRLN_MODULE
	TSLIB_MODULE_INIT(cy8mrln_palmpre_mod_init);
#endif

// vim: set noexpandtab shiftwidth=4 tabstop=4 ts=4:
