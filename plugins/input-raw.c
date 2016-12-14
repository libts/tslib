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
#include <string.h>
#include <stdint.h>

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
#ifndef SYN_CNT
# define SYN_CNT (SYN_MAX+1)
#endif

#ifndef ABS_MT_POSITION_X
# define ABS_MT_POSITION_X       0x35    /* Center X ellipse position */
# define ABS_MT_POSITION_Y       0x36    /* Center Y ellipse position */
#endif

#include "tslib-private.h"

#define GRAB_EVENTS_WANTED	1
#define GRAB_EVENTS_ACTIVE	2

#define NUM_EVENTS_READ 1

struct tslib_input {
	struct tslib_module_info module;

	int	current_x;
	int	current_y;
	int	current_p;

	int	using_syn;
	int	grab_events;

	struct input_event ev[NUM_EVENTS_READ];
	struct ts_sample_mt **buf;

	int	slot;
	int	max_slots;
	int	nr;
	int	pen_down;
	short	mt;
	int	last_fd;
	short	no_pressure;
	short	type_a;
};

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BIT(nr)                 (1UL << (nr))
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BITS_PER_BYTE           8
#define BITS_PER_LONG           (sizeof(long) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

static void set_pressure(struct tslib_input *i)
{
	int j, k;

	i->current_p = 255;

	if (i->mt && i->buf) {
		for (j = 0; j < i->nr; j++) {
			for (k = 0; k < i->max_slots; k++)
				i->buf[j][k].pressure = 255;
		}
	}
}

static int check_fd(struct tslib_input *i)
{
	struct tsdev *ts = i->module.dev;
	int version;
	long evbit[BITS_TO_LONGS(EV_CNT)];
	long absbit[BITS_TO_LONGS(ABS_CNT)];
	long keybit[BITS_TO_LONGS(KEY_CNT)];
	long synbit[BITS_TO_LONGS(SYN_CNT)];

	if (ioctl(ts->fd, EVIOCGVERSION, &version) < 0) {
		fprintf(stderr, "tslib: Selected device is not a Linux input event device\n");
		return -1;
	}

	/* support version EV_VERSION 0x010000 and 0x010001
	 * this check causes more troubles than it solves here */
	if (version < EV_VERSION)
		fprintf(stderr, "tslib: Warning: Selected device uses a different version of the event protocol than tslib was compiled for\n");

	if ( (ioctl(ts->fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) ||
		!(evbit[BIT_WORD(EV_ABS)] & BIT_MASK(EV_ABS)) ||
		!(evbit[BIT_WORD(EV_KEY)] & BIT_MASK(EV_KEY)) ) {
		fprintf(stderr, "tslib: Selected device is not a touchscreen (must support ABS and KEY event types)\n");
		return -1;
	}

	if ((ioctl(ts->fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit)) < 0 ||
	    !(absbit[BIT_WORD(ABS_X)] & BIT_MASK(ABS_X)) ||
	    !(absbit[BIT_WORD(ABS_Y)] & BIT_MASK(ABS_Y))) {
		if (!(absbit[BIT_WORD(ABS_MT_POSITION_X)] & BIT_MASK(ABS_MT_POSITION_X)) ||
		    !(absbit[BIT_WORD(ABS_MT_POSITION_Y)] & BIT_MASK(ABS_MT_POSITION_Y))) {
			fprintf(stderr, "tslib: Selected device is not a touchscreen (must support ABS_X/Y or ABS_MT_POSITION_X/Y events)\n");
			return -1;
		}
	}

	/* Since some touchscreens (eg. infrared) physically can't measure pressure,
	 * the input system doesn't report it on those. Tslib relies on pressure, thus
	 * we set it to constant 255. It's still controlled by BTN_TOUCH/BTN_LEFT -
	 * when not touched, the pressure is forced to 0. */
	if (!(absbit[BIT_WORD(ABS_PRESSURE)] & BIT_MASK(ABS_PRESSURE)))
		i->no_pressure = 1;

	if ((ioctl(ts->fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) ||
		!(keybit[BIT_WORD(BTN_TOUCH)] & BIT_MASK(BTN_TOUCH) ||
		  keybit[BIT_WORD(BTN_LEFT)] & BIT_MASK(BTN_LEFT))) {
		fprintf(stderr, "tslib: Selected device is not a touchscreen (must support BTN_TOUCH or BTN_LEFT events)\n");
		return -1;
	}

	/* Remember whether we have a multitouch device. We need to know for ABS_X,
	 * ABS_Y and ABS_PRESSURE data. */
	if ((absbit[BIT_WORD(ABS_MT_POSITION_X)] & BIT_MASK(ABS_MT_POSITION_X)) &&
	    (absbit[BIT_WORD(ABS_MT_POSITION_Y)] & BIT_MASK(ABS_MT_POSITION_Y)))
		i->mt = 1;

	/* remember if we have a device that doesn't support pressure. We have to
	 * set pressure ourselves in this case. */
	if (i->mt && !(absbit[BIT_WORD(ABS_MT_PRESSURE)] & BIT_MASK(ABS_MT_PRESSURE)))
		i->no_pressure = 1;

	if (evbit[BIT_WORD(EV_SYN)] & BIT_MASK(EV_SYN))
		i->using_syn = 1;

	if ((ioctl(ts->fd, EVIOCGBIT(EV_SYN, sizeof(synbit)), synbit)) == -1)
		fprintf(stderr, "tslib: ioctl error\n");

	/* remember whether we have a multitouch type A device */
	if (i->mt && synbit[BIT_WORD(SYN_MT_REPORT)] & BIT_MASK(SYN_MT_REPORT) &&
	    !(absbit[BIT_WORD(ABS_MT_SLOT)] & BIT_MASK(ABS_MT_SLOT)))
		i->type_a = 1;

	if (i->grab_events == GRAB_EVENTS_WANTED) {
		if (ioctl(ts->fd, EVIOCGRAB, (void *)1)) {
			fprintf(stderr, "tslib: Unable to grab selected input device\n");
			return -1;
		}
		i->grab_events = GRAB_EVENTS_ACTIVE;
	}

	return ts->fd;
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

	if (ts->fd != i->last_fd)
		i->last_fd = check_fd(i);

	if (i->last_fd == -1)
		return 0;

	if (i->no_pressure)
		set_pressure(i);

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
				fprintf(stderr, "RAW---------------------> %d %d %d %ld.%ld\n",
						samp->x, samp->y, samp->pressure, (long)samp->tv.tv_sec,
						(long)samp->tv.tv_usec);
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

static int ts_input_read_mt(struct tslib_module_info *inf,
			    struct ts_sample_mt **samp, int max_slots, int nr)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;
	int ret = nr;
	int total = 0;
	unsigned int it;
	int rd;
	int j, k;
	int pen_up = 0;

	if (i->buf == NULL) {
		i->buf = malloc(nr * sizeof(struct ts_sample_mt *));
		if (!i->buf)
			return -ENOMEM;

		for (j = 0; j < nr; j++) {
			i->buf[j] = malloc(max_slots * sizeof(struct ts_sample_mt));
			if (!i->buf[j])
				return -ENOMEM;
		}

		i->max_slots = max_slots;
		i->nr = nr;
	}

	if (ts->fd != i->last_fd)
		i->last_fd = check_fd(i);

	if (i->last_fd == -1)
		return 0;

	if (i->no_pressure)
		set_pressure(i);

	for (j = 0; j < nr; j++) {
		for (k = 0; k < max_slots; k++) {
			i->buf[j][k].valid = 0;
			i->buf[j][k].pen_down = -1;
		}
	}

	if (i->using_syn) {
		while (total < nr) {
			memset(i->ev, 0, sizeof(i->ev));

			rd = read(ts->fd, i->ev, sizeof(struct input_event) * NUM_EVENTS_READ);
			if (rd < (int) sizeof(struct input_event)) {
				perror("tslib: error reading input event");
				total = -1;
				break;
			}

			for (it = 0; it < rd / sizeof(struct input_event); it++) {
			#ifdef DEBUG
				printf("INPUT-RAW: read type %d  code %d  value %d\n",
				       i->ev[it].type, i->ev[it].code, i->ev[it].value);
			#endif
				switch(i->ev[it].type) {
				case EV_KEY:
					switch (i->ev[it].code) {
					case BTN_TOUCH:
					case BTN_LEFT:
						if (i->ev[it].code == BTN_TOUCH) {
							i->buf[total][i->slot].pen_down = i->ev[it].value;
							i->buf[total][i->slot].tv = i->ev[it].time;
							if (i->ev[it].value == 0)
								pen_up = 1;
						}
						break;
					}
					break;
				case EV_SYN:
					switch (i->ev[it].code) {
					case SYN_REPORT:
						if (pen_up && i->no_pressure) {
							for (k = 0; k < max_slots; k++) {
								if (i->buf[total][k].x != 0 ||
								    i->buf[total][k].y != 0 ||
								    i->buf[total][k].pressure != 0)
									i->buf[total][i->slot].valid = 1;

								i->buf[total][k].x = 0;
								i->buf[total][k].y = 0;
								i->buf[total][k].pressure = 0;
							}

						if (pen_up)
							pen_up = 0;
						}

						for (j = 0; j < nr; j++) {
							for (k = 0; k < max_slots; k++) {
								memcpy(&samp[j][k],
									&i->buf[j][k],
									sizeof(struct ts_sample_mt));
							}
						}

						if (i->type_a)
							i->slot = 0;

						total++;
						break;
					case SYN_MT_REPORT:
						if (i->type_a && i->slot < (max_slots - 1))
							i->slot++;
						break;
					}
					break;
				case EV_ABS:
					switch (i->ev[it].code) {
					case ABS_X:
						/* in case we didn't already get data for this
						 * slot, we go ahead and act as if this would be
						 * ABS_MT_POSITION_X
						 */
						if (i->mt && i->buf[total][i->slot].valid == 1)
							break;
					case ABS_MT_POSITION_X:
						i->buf[total][i->slot].x = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_Y:
						if (i->mt && i->buf[total][i->slot].valid == 1)
							break;
					case ABS_MT_POSITION_Y:
						i->buf[total][i->slot].y = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_PRESSURE:
						if (i->mt && i->buf[total][i->slot].valid == 1)
							break;
					case ABS_MT_PRESSURE:
						i->buf[total][i->slot].pressure = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					/* TODO transform tool_x/y too? */
					case ABS_MT_TOOL_X:
						i->buf[total][i->slot].tool_x = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_MT_TOOL_Y:
						i->buf[total][i->slot].tool_y = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_MT_TOOL_TYPE:
						i->buf[total][i->slot].tool_type = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_MT_ORIENTATION:
						i->buf[total][i->slot].orientation = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_MT_DISTANCE:
						i->buf[total][i->slot].distance = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_MT_BLOB_ID:
						i->buf[total][i->slot].blob_id = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_MT_TOUCH_MAJOR:
						i->buf[total][i->slot].touch_major = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_MT_WIDTH_MAJOR:
						i->buf[total][i->slot].width_major = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_MT_TOUCH_MINOR:
						i->buf[total][i->slot].touch_minor = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_MT_WIDTH_MINOR:
						i->buf[total][i->slot].width_minor = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						break;
					case ABS_MT_TRACKING_ID:
						i->buf[total][i->slot].tracking_id = i->ev[it].value;
						i->buf[total][i->slot].tv = i->ev[it].time;
						i->buf[total][i->slot].valid = 1;
						if (i->ev[it].value == -1)
							i->buf[total][i->slot].pressure = 0;
						break;
					case ABS_MT_SLOT:
						if (i->ev[it].value < 0 || i->ev[it].value >= max_slots) {
							fprintf(stderr, "tslib: slot out of range\n");
							return -EINVAL;
						}
						i->slot = i->ev[it].value;
						i->buf[total][i->slot].slot = i->ev[it].value;
						i->buf[total][i->slot].valid = 1;
						break;
					}
					break;
				}
				if (total == nr)
					break;
			}
		}
		ret = total;
	} else {
		/*
		 * XXX This could be simplified because it duplicates the above ts_read()
		 * code for ts_read_mt() to stay compatible and also support single touch
		 * protocols without EV_SYN.
		 */
		struct input_event ev_single;
		unsigned char *p = (unsigned char *) &ev_single;
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

			if (ev_single.type == EV_ABS) {
				switch (ev_single.code) {
				case ABS_X:
					if (ev_single.value != 0) {
						samp[total][0].x = i->current_x = ev_single.value;
						samp[total][0].y = i->current_y;
						samp[total][0].pressure = i->current_p;
					} else {
						fprintf(stderr, "tslib: dropped x = 0\n");
						continue;
					}
					break;
				case ABS_Y:
					if (ev_single.value != 0) {
						samp[total][0].x = i->current_x;
						samp[total][0].y = i->current_y = ev_single.value;
						samp[total][0].pressure = i->current_p;
					} else {
						fprintf(stderr, "tslib: dropped y = 0\n");
						continue;
					}
					break;
				case ABS_PRESSURE:
					samp[total][0].x = i->current_x;
					samp[total][0].y = i->current_y;
					samp[total][0].pressure = i->current_p = ev_single.value;
					break;
				}
				samp[total][0].tv = ev_single.time;
	#ifdef DEBUG
				fprintf(stderr, "RAW---------------------------> %d %d %d\n",
					samp[total][0].x, samp[total][0].y, samp[total][0].pressure);
	#endif /* DEBUG */
				samp++;
				total++;
			} else if (ev_single.type == EV_KEY) {
				switch (ev_single.code) {
				case BTN_TOUCH:
				case BTN_LEFT:
					if (ev_single.value == 0) {
						/* pen up */
						samp[total][0].x = 0;
						samp[total][0].y = 0;
						samp[total][0].pressure = 0;
						samp[total][0].tv = ev_single.time;
						total++;
					}
					break;
				}
			} else {
				fprintf(stderr, "tslib: Unknown event type %d\n", ev_single.type);
			}
			p = (unsigned char *) &ev_single;
		}
		ret = total;
	}

	return ret;
}

static int ts_input_fini(struct tslib_module_info *inf)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;
	int j;

	if (i->grab_events == GRAB_EVENTS_ACTIVE) {
		if (ioctl(ts->fd, EVIOCGRAB, (void *)0)) {
			fprintf(stderr, "tslib: Unable to un-grab selected input device\n");
		}
	}

	for (j = 0; j < i->nr; j++) {
		if (i->buf[j])
			free(i->buf[j]);
	}
	if (i->buf)
		free (i->buf);

	free(inf);

	return 0;
}

static const struct tslib_ops __ts_input_ops = {
	.read		= ts_input_read,
	.read_mt	= ts_input_read_mt,
	.fini		= ts_input_fini,
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
	switch ((int)(intptr_t)data) {
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

TSAPI struct tslib_module_info *input_mod_init(__attribute__ ((unused)) struct tsdev *dev,
					       const char *params)
{
	struct tslib_input *i;

	i = malloc(sizeof(struct tslib_input));
	if (i == NULL)
		return NULL;

	i->module.ops = &__ts_input_ops;
	i->current_x = 0;
	i->current_y = 0;
	i->current_p = 0;
	i->using_syn = 0;
	i->grab_events = 0;
	i->slot = 0;
	i->pen_down = 0;
	i->buf = NULL;
	i->max_slots = 0;
	i->nr = 0;
	i->mt = 0;
	i->no_pressure = 0;
	i->last_fd = -2;
	i->type_a = 0;

	if (tslib_parse_vars(&i->module, raw_vars, NR_VARS, params)) {
		free(i);
		return NULL;
	}

	return &(i->module);
}

#ifndef TSLIB_STATIC_INPUT_MODULE
	TSLIB_MODULE_INIT(input_mod_init);
#endif 
