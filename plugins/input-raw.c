/*
 *  tslib/src/ts_read_raw_module.c
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
 * $Id: input-raw.c,v 1.5 2005/02/26 01:47:23 kergoth Exp $
 *
 * Read raw pressure, x, y, and timestamp from a touchscreen device.
 */
#include "config.h"

#include <errno.h>
#include <stdio.h>

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

#include "tslib-private.h"

struct tslib_input {
	struct tslib_module_info module;

	int	current_x;
	int	current_y;
	int	current_p;

	int	sane_fd;
	int	using_syn;
};

static int check_fd(struct tslib_input *i)
{
	struct tsdev *ts = i->module.dev;
	int version;
	u_int32_t bit;
	u_int64_t absbit;

	if (! ((ioctl(ts->fd, EVIOCGVERSION, &version) >= 0) &&
		(version == EV_VERSION) &&
		(ioctl(ts->fd, EVIOCGBIT(0, sizeof(bit) * 8), &bit) >= 0) &&
		(bit & (1 << EV_ABS)) &&
		(ioctl(ts->fd, EVIOCGBIT(EV_ABS, sizeof(absbit) * 8), &absbit) >= 0) &&
		(absbit & (1 << ABS_X)) &&
		(absbit & (1 << ABS_Y)) && (absbit & (1 << ABS_PRESSURE)))) {
		fprintf(stderr, "selected device is not a touchscreen I understand\n");
		return -1;
	}

	if (bit & (1 << EV_SYN))
		i->using_syn = 1;

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
				break;
			case EV_SYN:
				/* Fill out a new complete event */
				samp->x = i->current_x;
				samp->y = i->current_y;
				samp->pressure = i->current_p;
				samp->tv = ev.time;
	#ifdef DEBUG
				fprintf(stderr, "RAW---------------------> %d %d %d %d.%d\n",
						samp->x, samp->y, samp->pressure, samp->tv.tv_sec,
						samp->tv.tv_usec);
	#endif		 /*DEBUG*/
					samp++;
				total++;
				break;
			case EV_ABS:
				switch (ev.code) {
				case ABS_X:
					i->current_x = ev.value;
					break;
				case ABS_Y:
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
	#endif	 /*DEBUG*/
				samp++;
				total++;
			} else if (ev.type == EV_KEY) {
				switch (ev.code) {
				case BTN_TOUCH:
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
	free(inf);
	return 0;
}

static const struct tslib_ops __ts_input_ops = {
	.read	= ts_input_read,
	.fini	= ts_input_fini,
};

TSAPI struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
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
	return &(i->module);
}
