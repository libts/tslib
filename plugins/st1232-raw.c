/*
 *  tslib/plugins/st1232-raw.c
 *
 * based on input-raw.c
 * 2015.06 Peter Vicman
 *
 *  Original version:
 *  Copyright (C) 2001 Russell King.
 *
 *  Rewritten for the Linux input device API:
 *  Copyright (C) 2002 Nicolas Pitre
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Plugin for "st1232-touchscreen" (found on 7" display for Udoo board with i.MX6 SoC)
 *
 * Read raw x, y, and timestamp from a touchscreen device.
 * Only first mt report is taken and send upstream.
 * ST1232 driver doesn't report ABS_X/Y events - only MT - and
 * doesn't report BTN_TOUCH event.
 *
 * In memory of my mom and dad ...
 */
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <limits.h>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/time.h>
#include <sys/types.h>

#include <linux/input.h>
#ifndef EV_SYN /* 2.4 kernel headers */
# define EV_SYN 0x00
#endif
#ifndef EV_CNT
# define EV_CNT (EV_MAX+1)
#endif
#ifndef ABS_CNT
# define ABS_CNT (ABS_MAX+1)
#endif
#ifndef KEY_CNT
# define KEY_CNT (KEY_MAX+1)
#endif

#ifndef ABS_MT_POSITION_X
# define ABS_MT_POSITION_X       0x35    /* Center X ellipse position */
# define ABS_MT_POSITION_Y       0x36    /* Center Y ellipse position */
#endif

#define SYN_MT_REPORT		2

#include "tslib-private.h"

#define GRAB_EVENTS_WANTED	1
#define GRAB_EVENTS_ACTIVE	2

struct tslib_input {
	struct tslib_module_info module;

	int	current_x;
	int	current_y;
	int	current_p;

	int	sane_fd;
	int	using_syn;
	int	grab_events;
	int invert_y;
};

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BIT(nr)                 (1UL << (nr))
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BITS_PER_BYTE           8
#define BITS_PER_LONG           (sizeof(long) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

static int check_fd(struct tslib_input *i)
{
	struct tsdev *ts = i->module.dev;
	int version;
	long evbit[BITS_TO_LONGS(EV_CNT)];
	long absbit[BITS_TO_LONGS(ABS_CNT)];

	if (ioctl(ts->fd, EVIOCGVERSION, &version) < 0) {
		fprintf(stderr, "tslib: Selected device is not a Linux input event device\n");
		return -1;
	}

	if (version < EV_VERSION) {
		fprintf(stderr, "tslib: Selected device uses a different version of the event protocol than tslib was compiled for\n");
		return -1;
	}

	if ( (ioctl(ts->fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) ||
		!(evbit[BIT_WORD(EV_ABS)] & BIT_MASK(EV_ABS)) ||
		!(evbit[BIT_WORD(EV_KEY)] & BIT_MASK(EV_KEY)) ) {
		fprintf(stderr, "tslib: Selected device is not a touchscreen (must support ABS and KEY event types)\n");
		return -1;
	}

	if ((ioctl(ts->fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit)) < 0 ||
		!(absbit[BIT_WORD(ABS_MT_POSITION_X)] & BIT_MASK(ABS_MT_POSITION_X)) ||
		!(absbit[BIT_WORD(ABS_MT_POSITION_Y)] & BIT_MASK(ABS_MT_POSITION_Y))) {
		fprintf(stderr, "tslib: Selected device is not a touchscreen (must support ABS_MT_POSITION_X and ABS_MT_POSITION_Y events)\n");
		return -1;
	}

	if (evbit[BIT_WORD(EV_SYN)] & BIT_MASK(EV_SYN))
		i->using_syn = 1;

	if (i->grab_events == GRAB_EVENTS_WANTED) {
		if (ioctl(ts->fd, EVIOCGRAB, (void *)1)) {
			fprintf(stderr, "tslib: Unable to grab selected input device\n");
			return -1;
		}
		i->grab_events = GRAB_EVENTS_ACTIVE;
	}

	return 0;
}

static int ts_st1232_read(struct tslib_module_info *inf,
			 struct ts_sample *samp, int nr)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;
	struct input_event ev;
	int ret = nr;
	int total = 0;
	int mt_cnt = 0;		/* data for first finger */

	if (i->sane_fd == -1)
		i->sane_fd = check_fd(i);

	if (i->sane_fd == -1)
		return 0;

	if (i->using_syn) {
		while (total < nr) {
			ret = read(ts->fd, &ev, sizeof(struct input_event));
			if (ret < (int) sizeof(struct input_event)) {
				total = -1;
				break;
			}

			switch (ev.type) {
				case EV_SYN:
					if (ev.code == SYN_MT_REPORT) {
						mt_cnt++;		/* data for next finger will come */
					} else if (ev.code == SYN_REPORT) {
						/* fill out a new complete event */
						/* always send coordinate (last one on finger released) */
						samp->x = i->current_x;
						samp->y = i->current_y;
						samp->pressure = i->current_p;	/* is 0 on finger released */
						samp->tv = ev.time;
      	
						i->current_p = 0;		/* will be set again when getting xy cordinate */					
						samp++;
						total++;
						mt_cnt = 0;
					}
					break;
				case EV_ABS:
					if (mt_cnt > 0)
						break;	/* save data only for first finger */
								
					switch (ev.code) {
						case ABS_MT_POSITION_X:					
							i->current_x = ev.value;
							i->current_p = 255;  /* touched */
							break;
						case ABS_MT_POSITION_Y:
							i->current_y = ev.value;
							i->current_p = 255;  /* touched */
      	
							if (i->invert_y > 0)
								i->current_y = i->invert_y - i->current_y;
							break;
					}
					break;
			}
		}
		
		ret = total;
	} else {
		fprintf(stderr, "tslib: st1232 and not using syn\n");
		sleep(1);
	}

	return ret;
}

static int ts_st1232_fini(struct tslib_module_info *inf)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;

	if (i->grab_events == GRAB_EVENTS_ACTIVE) {
		if (ioctl(ts->fd, EVIOCGRAB, (void *)0)) {
			fprintf(stderr, "tslib: Unable to un-grab selected input device\n");
		}
	}

	free(inf);
	return 0;
}

static const struct tslib_ops __ts_st1232_ops = {
	.read	= ts_st1232_read,
	.fini	= ts_st1232_fini,
};

static int parse_raw_grab(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul(str, NULL, 0);

	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)data) {
		case 1:
			if (v)
				i->grab_events = GRAB_EVENTS_WANTED;
			break;
		default:
			return -1;
	}
	return 0;
}

static int parse_invert_y(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	int v;
	int err = errno;

	v = atoi(str);

	if (v < 0)
		return -1;

	errno = err;
	switch ((int)data) {
		case 1:
			i->invert_y = v;
			break;
		default:
			return -1;
	}
	return 0;
}

static const struct tslib_vars raw_vars[] =
{
	{ "grab_events", (void *)1, parse_raw_grab },
	{ "invert_y", (void *)1, parse_invert_y },
};

#define NR_VARS (sizeof(raw_vars) / sizeof(raw_vars[0]))

TSAPI struct tslib_module_info *st1232_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_input *i;

	i = malloc(sizeof(struct tslib_input));
	if (i == NULL)
		return NULL;

	i->module.ops = &__ts_st1232_ops;
	i->current_x = 0;
	i->current_y = 0;
	i->current_p = 0;
	i->sane_fd = -1;
	i->using_syn = 0;
	i->grab_events = 0;
	i->invert_y = 0;

	if (tslib_parse_vars(&i->module, raw_vars, NR_VARS, params)) {
		free(i);
		return NULL;
	}

	return &(i->module);
}

#ifndef TSLIB_STATIC_ST1232_MODULE
	TSLIB_MODULE_INIT(st1232_mod_init);
#endif
