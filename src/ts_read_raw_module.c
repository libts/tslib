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
 * $Id: ts_read_raw_module.c,v 1.1 2004/07/21 19:17:18 dlowder Exp $
 *
 * Read raw pressure, x, y, and timestamp from a touchscreen device.
 */
#include "config.h"

#include <stdio.h>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/time.h>
#include <sys/types.h>

#ifdef USE_INPUT_API
#include <linux/input.h>
#else
#endif /* USE_INPUT_API */

#include "tslib-private.h"

static int __ts_read_raw(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
#ifdef USE_INPUT_API
	struct input_event ev;
#endif /* USE_INPUT_API */
	int ret = nr;
	int total = 0;

#ifdef USE_INPUT_API
#ifdef EV_SYN
        /* This version uses EV_SYN */
	while (total < nr) {
		ret = read(ts->fd, &ev, sizeof(struct input_event));
		if (ret < sizeof(struct input_event)) {
			total = -1;
			break;
		}

		switch (ev.type) {
		case EV_SYN:
			/* Fill out a new complete event */
			samp->x = ts->current_x;
			samp->y = ts->current_y;
			samp->pressure = ts->current_p;
			samp->tv = ev.time;
#ifdef DEBUG
			fprintf(stderr,"RAW---------------------> %d %d %d %d.%d\n",
				samp->x, samp->y, samp->pressure, samp->tv.tv_sec, samp->tv.tv_usec);
#endif /*DEBUG*/
			samp++;
			total++;
			break;
		case EV_ABS:
			switch (ev.code) {
			case ABS_X:
				ts->current_x = ev.value;
				break;
			case ABS_Y:
				ts->current_y = ev.value;
				break;
			case ABS_PRESSURE:
				ts->current_p = ev.value;
				break;
			}
			break;
		}
	}
	ret = total;
#else   /* This version doesn't use EV_SYN */
	/* warning: maybe those static vars should be part of the tsdev struct? */
	static int curr_x = 0, curr_y = 0, curr_p = 0;
	static int got_curr_x = 0, got_curr_y = 0;
	int got_curr_p = 0;
	int next_x, next_y;
	int got_next_x = 0, got_next_y = 0;
	int got_tstamp = 0;

	while (total < nr) {
		ret = read(ts->fd, &ev, sizeof(struct input_event));
		if (ret < sizeof(struct input_event)) break;

		/*
		 * We must filter events here.  We need to look for
		 * a set of input events that will correspond to a
		 * complete ts event.  Also need to be aware that
		 * repeated input events are filtered out by the kernel.
		 * 
		 * We assume the normal sequence is: 
		 * ABS_X -> ABS_Y -> ABS_PRESSURE
		 * If that sequence goes backward then we got a different
		 * ts event.  If some are missing then they didn't change.
		 */
		if (ev.type == EV_ABS) switch (ev.code) {
		case ABS_X:
			if (!got_curr_x && !got_curr_y) {
				got_curr_x = 1;
				curr_x = ev.value;
			} else {
				got_next_x = 1;
				next_x = ev.value;
			}
			break;
		case ABS_Y:
			if (!got_curr_y) {
				got_curr_y = 1;
				curr_y = ev.value;
			} else {
				got_next_y = 1;
				next_y = ev.value;
			}
			break;
		case ABS_PRESSURE:
			got_curr_p = 1;
			curr_p = ev.value;
			break;
		}

		/* go back if we just got irrelevant events so far */
		if (!got_curr_x && !got_curr_y && !got_curr_p) continue;

		/* time stamp with the first valid event only */
		if (!got_tstamp) {
			got_tstamp = 1;
			samp->tv = ev.time;
		}

		if ( (!got_curr_x || !got_curr_y) && !got_curr_p &&
		     !got_next_x && !got_next_y ) {
			/*
			 * The current event is not complete yet.
			 * Give the kernel a chance to feed us more.
			 */
			struct timeval tv = {0, 0};
			fd_set fdset;
			FD_ZERO(&fdset);
			FD_SET(ts->fd, &fdset);
			ret = select(ts->fd+1, &fdset, NULL, NULL, &tv);
		       	if (ret == 1) continue;
			if (ret == -1) break;
		}

		/* We consider having a complete ts event */
		samp->x = curr_x;
		samp->y = curr_y;
		samp->pressure = curr_p;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
		samp++;
		total++;
        
		/* get ready for next event */
		if (got_next_x) curr_x = next_x; else got_curr_x = 0;
		if (got_next_y) curr_y = next_y; else got_curr_y = 0;
		got_next_x = got_next_y = got_tstamp = 0;
	}

	if (ret) ret = -1;
	if (total) ret = total;
#endif
#endif /* USE_INPUT_API */

	return ret;
}

static const struct tslib_ops __ts_raw_ops =
{
	read:	__ts_read_raw,
};

struct tslib_module_info __ts_raw =
{
	next:	NULL,
	ops:	&__ts_raw_ops,
};
