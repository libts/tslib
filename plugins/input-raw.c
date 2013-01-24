/*
 *  tslib/plugins/input-raw.c
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
 * Read raw pressure, x, y, and timestamp from a touchscreen device.
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
	long keybit[BITS_TO_LONGS(KEY_CNT)];

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
		!(absbit[BIT_WORD(ABS_X)] & BIT_MASK(ABS_X)) ||
		!(absbit[BIT_WORD(ABS_Y)] & BIT_MASK(ABS_Y))) {
		fprintf(stderr, "tslib: Selected device is not a touchscreen (must support ABS_X and ABS_Y events)\n");
		return -1;
	}

	/* Since some touchscreens (eg. infrared) physically can't measure pressure,
	the input system doesn't report it on those. Tslib relies on pressure, thus
	we set it to constant 255. It's still controlled by BTN_TOUCH/BTN_LEFT -
	when not touched, the pressure is forced to 0. */

	if (!(absbit[BIT_WORD(ABS_PRESSURE)] & BIT_MASK(ABS_PRESSURE))) {
		i->current_p = 255;

		if ((ioctl(ts->fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) ||
			!(keybit[BIT_WORD(BTN_TOUCH)] & BIT_MASK(BTN_TOUCH) ||
			  keybit[BIT_WORD(BTN_LEFT)] & BIT_MASK(BTN_LEFT))) {
			fprintf(stderr, "tslib: Selected device is not a touchscreen (must support BTN_TOUCH or BTN_LEFT events)\n");
			return -1;
		}
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

static int ts_input_read(struct tslib_module_info *inf,
			 struct ts_sample *samp, int nr)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;
	struct input_event ev;
	int ret = nr;
	int total = 0;
	int pen_up = 0;

	if (i->sane_fd == 0)
		i->sane_fd = check_fd(i);

	if (i->sane_fd == -1)
		return 0;

	if (i->using_syn) {
		while (total < nr) {
			ret = read(ts->fd, &ev, sizeof(struct input_event));
			if (ret < (int)sizeof(struct input_event)) {
				total = -1;
				break;
			}

			switch (ev.type) {
			case EV_KEY:
				switch (ev.code) {
				case BTN_TOUCH:
				case BTN_LEFT:
					if (ev.value == 0)
						pen_up = 1;
					break;
				}
				break;
			case EV_SYN:
				if (ev.code == SYN_REPORT) {
					/* Fill out a new complete event */
					if (pen_up) {
						samp->x = 0;
						samp->y = 0;
						samp->pressure = 0;
						pen_up = 0;
					} else {
						samp->x = i->current_x;
						samp->y = i->current_y;
						samp->pressure = i->current_p;
				}
				samp->tv = ev.time;
	#ifdef DEBUG
				fprintf(stderr, "RAW---------------------> %d %d %d %d.%d\n",
						samp->x, samp->y, samp->pressure, samp->tv.tv_sec,
						samp->tv.tv_usec);
	#endif /* DEBUG */
				samp++;
				total++;
				}
				break;
			case EV_ABS:
				switch (ev.code) {
				case ABS_X:
					i->current_x = ev.value;
					break;
				case ABS_Y:
					i->current_y = ev.value;
					break;
				case ABS_MT_POSITION_X:
					i->current_x = ev.value;
					break;
				case ABS_MT_POSITION_Y:
					i->current_y = ev.value;
					break;
				case ABS_PRESSURE:
					i->current_p = ev.value;
					break;
				}
				break;
			}
		}
		ret = total;
	} else {
		unsigned char *p = (unsigned char *) &ev;
		int len = sizeof(struct input_event);

		while (total < nr) {
			ret = read(ts->fd, p, len);
			if (ret == -1) {
				if (errno == EINTR) {
					continue;
				}
				break;
			}

			if (ret < (int)sizeof(struct input_event)) {
				/* short read
				 * restart read to get the rest of the event
				 */
				p += ret;
				len -= ret;
				continue;
			}
			/* successful read of a whole event */

			if (ev.type == EV_ABS) {
				switch (ev.code) {
				case ABS_X:
					if (ev.value != 0) {
						samp->x = i->current_x = ev.value;
						samp->y = i->current_y;
						samp->pressure = i->current_p;
					} else {
						fprintf(stderr, "tslib: dropped x = 0\n");
						continue;
					}
					break;
				case ABS_Y:
					if (ev.value != 0) {
						samp->x = i->current_x;
						samp->y = i->current_y = ev.value;
						samp->pressure = i->current_p;
					} else {
						fprintf(stderr, "tslib: dropped y = 0\n");
						continue;
					}
					break;
				case ABS_PRESSURE:
					samp->x = i->current_x;
					samp->y = i->current_y;
					samp->pressure = i->current_p = ev.value;
					break;
				}
				samp->tv = ev.time;
	#ifdef DEBUG
				fprintf(stderr, "RAW---------------------------> %d %d %d\n",
					samp->x, samp->y, samp->pressure);
	#endif /* DEBUG */
				samp++;
				total++;
			} else if (ev.type == EV_KEY) {
				switch (ev.code) {
				case BTN_TOUCH:
				case BTN_LEFT:
					if (ev.value == 0) {
						/* pen up */
						samp->x = 0;
						samp->y = 0;
						samp->pressure = 0;
						samp->tv = ev.time;
						samp++;
						total++;
					}
					break;
				}
			} else {
				fprintf(stderr, "tslib: Unknown event type %d\n", ev.type);
			}
			p = (unsigned char *) &ev;
		}
		ret = total;
	}

	return ret;
}

static int ts_input_fini(struct tslib_module_info *inf)
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

static const struct tslib_ops __ts_input_ops = {
	.read	= ts_input_read,
	.fini	= ts_input_fini,
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

static const struct tslib_vars raw_vars[] =
{
	{ "grab_events", (void *)1, parse_raw_grab },
};

#define NR_VARS (sizeof(raw_vars) / sizeof(raw_vars[0]))

TSAPI struct tslib_module_info *input_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_input *i;

	i = malloc(sizeof(struct tslib_input));
	if (i == NULL)
		return NULL;

	i->module.ops = &__ts_input_ops;
	i->current_x = 0;
	i->current_y = 0;
	i->current_p = 0;
	i->sane_fd = 0;
	i->using_syn = 0;
	i->grab_events = 0;

	if (tslib_parse_vars(&i->module, raw_vars, NR_VARS, params)) {
		free(i);
		return NULL;
	}

	return &(i->module);
}

#ifndef TSLIB_STATIC_INPUT_MODULE
	TSLIB_MODULE_INIT(input_mod_init);
#endif 
