/*
 *  tslib/plugins/input-raw.c
 *
 *  Additions for the Linux multitouch API:
 *  Copyright (C) 2016 Martin Kepplinger <martin.kepplinger@ginzinger.com>
 *
 *  Rewritten for the Linux input device API:
 *  Copyright (C) 2002 Nicolas Pitre
 *
 *  Original version:
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
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
# include <unistd.h>
#endif

#include <sys/time.h>
#include <sys/types.h>

#ifdef __FreeBSD__
# include <dev/evdev/input.h>
#else
# include <linux/input.h>
#endif

#ifndef input_event_sec
#define input_event_sec time.tv_sec
#define input_event_usec time.tv_usec
#endif

#ifndef EV_SYN /* 2.4 kernel headers */
# define EV_SYN 0x00
#endif
#ifndef EV_CNT
# define EV_CNT (EV_MAX+1)
#endif
#ifndef ABS_CNT /* < 2.6.24 kernel headers */
# define ABS_CNT (ABS_MAX+1)
#endif
#ifndef KEY_CNT
# define KEY_CNT (KEY_MAX+1)
#endif
#ifndef SYN_MT_REPORT /* < 2.6.30 kernel headers */
# define SYN_MT_REPORT 2
#endif
#ifndef SYN_DROPPED /* < 2.6.39 kernel headers */
# define SYN_DROPPED 3
#endif
#ifndef SYN_MAX /* < 3.12 kernel headers */
# define SYN_MAX 0xf
#endif
#ifndef SYN_CNT
# define SYN_CNT (SYN_MAX+1)
#endif

#ifndef ABS_MT_SLOT /* < 2.6.36 kernel headers */
# define ABS_MT_SLOT             0x2f    /* MT slot being modified */
#endif
#ifndef ABS_MT_POSITION_X /* < 2.6.30 kernel headers */
# define ABS_MT_TOUCH_MAJOR      0x30    /* Major axis of touching ellipse */
# define ABS_MT_TOUCH_MINOR      0x31    /* Minor axis (omit if circular) */
# define ABS_MT_WIDTH_MAJOR      0x32    /* Major axis of approaching ellipse */
# define ABS_MT_WIDTH_MINOR      0x33    /* Minor axis (omit if circular) */
# define ABS_MT_ORIENTATION      0x34    /* Ellipse orientation */
# define ABS_MT_POSITION_X       0x35    /* Center X touch position */
# define ABS_MT_POSITION_Y       0x36    /* Center Y touch position */
# define ABS_MT_TOOL_TYPE        0x37    /* Type of touching device */
# define ABS_MT_BLOB_ID          0x38    /* Group a set of packets as a blob */
# define ABS_MT_TRACKING_ID      0x39    /* Unique ID of initiated contact */
#endif
#ifndef ABS_MT_PRESSURE /* < 2.6.33 kernel headers */
# define ABS_MT_PRESSURE         0x3a    /* Pressure on contact area */
#endif
#ifndef ABS_MT_DISTANCE /* < 2.6.38 kernel headers */
# define ABS_MT_DISTANCE         0x3b    /* Contact hover distance */
#endif
#ifndef ABS_MT_TOOL_X /* < 3.6 kernel headers */
# define ABS_MT_TOOL_X           0x3c    /* Center X tool position */
# define ABS_MT_TOOL_Y           0x3d    /* Center Y tool position */
#endif

#include "tslib-private.h"

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define BIT(nr)                 (1UL << (nr))
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BITS_PER_BYTE           8
#define BITS_PER_LONG           (sizeof(long) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

#define GRAB_EVENTS_WANTED	1
#define GRAB_EVENTS_ACTIVE	2

#define NUM_EVENTS_READ 1 /* internal. independent from the user call */

struct tslib_input {
	struct tslib_module_info module;

	int	current_x;
	int	current_y;
	int	current_p;

	int8_t	using_syn;
	int8_t	grab_events;

	struct input_event ev[NUM_EVENTS_READ];
	struct ts_sample_mt **buf;

	int	slot;
	int	max_slots;
	int	nr;
	int	pen_down;
	int	last_fd;
	int8_t	mt;
	int8_t	no_pressure;
	int8_t	type_a;
	int32_t *last_pressure;
	int8_t	last_type_a_slots;

	uint16_t	special_device; /* broken device we work around, see below */
};

#ifndef BUS_USB
#define BUS_USB 0x03
#endif

/* List of VIDs we use special cases for */
#define USB_VID_EGALAX 0x0EEF

/* List of actual devices we enumerate. For the device selection,
 * see get_special_device()
 */
#define EGALAX_VERSION_210 2

static int get_special_device(struct tslib_input *i)
{
	struct input_id id;
	struct tsdev *ts = i->module.dev;

	if ((ioctl(ts->fd, EVIOCGID, &id) < 0)) {
		fprintf(stderr, "tslib: warning, can't read device id\n");
		return -1;
	}

#ifdef DEBUG
	printf("tslib input device: vendor 0x%X product 0x%X version 0x%X on bus 0x%X\n",
	       id.vendor, id.product, id.version, id.bustype);
#endif

	/* only special usb devices so far, see the list above.
	 * so if not usb, we are done. */
	if (id.bustype != BUS_USB)
		return 0;

	switch (id.vendor) {
	case USB_VID_EGALAX:
		switch (id.product) {
		case 0x0001:
		/* taken from galax-raw. is this correct? */
		case 0x7200:
		case 0x7201:
			/* please note that any workarounds that don't
			 * apply to only a specific VERSION of one product,
			 * but to the product ID, should *really* be handled
			 * in the kernel! They have quirks over there.
			 */
			switch (id.version) {
			case 0x0210:
				i->special_device = EGALAX_VERSION_210;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return 0;
}

static void set_pressure(struct tslib_input *i)
{
	int j, k;

	i->current_p = 255;

	if (i->buf) {
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
		fprintf(stderr,
			"tslib: Selected device is not a Linux input event device\n");
		return -1;
	}

	/* support version EV_VERSION 0x010000 and 0x010001
	 * this check causes more troubles than it solves here
	 */
	if (version < EV_VERSION)
		fprintf(stderr,
			"tslib: Warning: Selected device uses a different version of the event protocol than tslib was compiled for\n");

	if ((ioctl(ts->fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) ||
		!(evbit[BIT_WORD(EV_ABS)] & BIT_MASK(EV_ABS))) {
		fprintf(stderr,
			"tslib: Selected device is not a touchscreen (must support ABS event type)\n");
		return -1;
	}

	if ((ioctl(ts->fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit)) < 0 ||
	    !(absbit[BIT_WORD(ABS_X)] & BIT_MASK(ABS_X)) ||
	    !(absbit[BIT_WORD(ABS_Y)] & BIT_MASK(ABS_Y))) {
		if (!(absbit[BIT_WORD(ABS_MT_POSITION_X)] & BIT_MASK(ABS_MT_POSITION_X)) ||
		    !(absbit[BIT_WORD(ABS_MT_POSITION_Y)] & BIT_MASK(ABS_MT_POSITION_Y))) {
			fprintf(stderr,
				"tslib: Selected device is not a touchscreen (must support ABS_X/Y or ABS_MT_POSITION_X/Y events)\n");
			return -1;
		}
	}

	/* Remember whether we have a multitouch device. We need to know for ABS_X,
	 * ABS_Y and ABS_PRESSURE data.
	 */
	if ((absbit[BIT_WORD(ABS_MT_POSITION_X)] & BIT_MASK(ABS_MT_POSITION_X)) &&
	    (absbit[BIT_WORD(ABS_MT_POSITION_Y)] & BIT_MASK(ABS_MT_POSITION_Y)))
		i->mt = 1;

	if (evbit[BIT_WORD(EV_KEY)] & BIT_MASK(EV_KEY)) {
		if (ioctl(ts->fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) {
			fprintf(stderr, "tslib: ioctl EVIOCGBIT error)\n");
			return -1;
		}

		/* for multitouch type B, tracking id is enough for pen down/up. type A
		 * has pen down/up through the list of (empty) SYN_MT_REPORT
		 * only for singletouch we need BTN_TOUCH or BTN_LEFT
		 */
		if (!(keybit[BIT_WORD(BTN_TOUCH)] & BIT_MASK(BTN_TOUCH) ||
		      keybit[BIT_WORD(BTN_LEFT)] & BIT_MASK(BTN_LEFT)) && !i->mt) {
			fprintf(stderr,
				"tslib: Selected device is not a touchscreen (missing BTN_TOUCH or BTN_LEFT)\n");
			return -1;
		}
	}

	if (evbit[BIT_WORD(EV_SYN)] & BIT_MASK(EV_SYN))
		i->using_syn = 1;

	/* read device info and set special device nr */
	get_special_device(i);

	/* Since some touchscreens (eg. infrared) physically can't measure pressure,
	 * the input system doesn't report it on those. Tslib relies on pressure, thus
	 * we set it to constant 255. It's still controlled by BTN_TOUCH/BTN_LEFT -
	 * when not touched, the pressure is forced to 0.
	 */
	if (!(absbit[BIT_WORD(ABS_PRESSURE)] & BIT_MASK(ABS_PRESSURE)))
		i->no_pressure = 1;

	if (i->mt) {
		if (!(absbit[BIT_WORD(ABS_MT_PRESSURE)] & BIT_MASK(ABS_MT_PRESSURE)))
			i->no_pressure = 1;
		else
			i->no_pressure = 0;
	}
#ifdef DEBUG
	if (i->no_pressure) {
		printf("tslib: No pressure values. We fake it internally\n");
	} else {
		printf("tslib: We have a pressure value\n");
	}
#endif

	if ((ioctl(ts->fd, EVIOCGBIT(EV_SYN, sizeof(synbit)), synbit)) == -1)
		fprintf(stderr, "tslib: ioctl error\n");

	/* remember whether we have a multitouch type A device */
	if (i->mt &&
	    !(absbit[BIT_WORD(ABS_MT_SLOT)] & BIT_MASK(ABS_MT_SLOT)) &&
	    !(absbit[BIT_WORD(ABS_MT_TRACKING_ID)] & BIT_MASK(ABS_MT_TRACKING_ID))) {
		i->type_a = 1;
	#ifdef DEBUG
		printf("tslib: We have a multitouch type A device\n");
	#endif
	}

	if (i->grab_events == GRAB_EVENTS_WANTED) {
		if (ioctl(ts->fd, EVIOCGRAB, (void *)1)) {
			fprintf(stderr,
				"tslib: Unable to grab selected input device\n");
			return -1;
		}
		i->grab_events = GRAB_EVENTS_ACTIVE;
	}

	if (i->mt && !i->using_syn) {
		fprintf(stderr,
			"tslib: Unsupported multitouch device (missing EV_SYN)\n");
		return -1;
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
		return -ENODEV;

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
					samp->tv.tv_sec = ev.input_event_sec;
					samp->tv.tv_usec = ev.input_event_usec;
			#ifdef DEBUG
				fprintf(stderr,
					"RAW---------------------> %d %d %d %ld.%ld\n",
					samp->x, samp->y, samp->pressure,
					(long)samp->tv.tv_sec,
					(long)samp->tv.tv_usec);
			#endif /* DEBUG */
					samp++;
					total++;
				} else if (ev.code == SYN_MT_REPORT) {
					if (!i->type_a)
						break;

					if (i->type_a == 1) { /* no data: pen-up */
						pen_up = 1;

					} else {
						i->type_a = 1;
					}
			#ifdef DEBUG
				} else if (ev.code == SYN_DROPPED) {
					fprintf(stderr,
						"INPUT-RAW: SYN_DROPPED\n");
			#endif
				}
				break;
			case EV_ABS:
				if (i->special_device == EGALAX_VERSION_210) {
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
					case ABS_MT_DISTANCE:
						if (ev.value > 0)
							i->current_p = 0;
						else
							i->current_p = 255;
						break;
					}
				} else {
					switch (ev.code) {
					case ABS_X:
						i->current_x = ev.value;
						break;
					case ABS_Y:
						i->current_y = ev.value;
						break;
					case ABS_MT_POSITION_X:
						i->current_x = ev.value;
						i->type_a++;
						break;
					case ABS_MT_POSITION_Y:
						i->current_y = ev.value;
						i->type_a++;
						break;
					case ABS_PRESSURE:
						i->current_p = ev.value;
						break;
					case ABS_MT_PRESSURE:
						i->current_p = ev.value;
						break;
					case ABS_MT_TOUCH_MAJOR:
						if (ev.value == 0)
							i->current_p = 0;
						break;
					case ABS_MT_TRACKING_ID:
						if (ev.value == -1)
							i->current_p = 0;
						break;
					}
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
				if (errno == EINTR)
					continue;

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
						fprintf(stderr,
							"tslib: dropped x = 0\n");
						continue;
					}
					break;
				case ABS_Y:
					if (ev.value != 0) {
						samp->x = i->current_x;
						samp->y = i->current_y = ev.value;
						samp->pressure = i->current_p;
					} else {
						fprintf(stderr,
							"tslib: dropped y = 0\n");
						continue;
					}
					break;
				case ABS_PRESSURE:
					samp->x = i->current_x;
					samp->y = i->current_y;
					samp->pressure = i->current_p = ev.value;
					break;
				}
				samp->tv.tv_sec = ev.input_event_sec;
				samp->tv.tv_usec = ev.input_event_usec;
	#ifdef DEBUG
				fprintf(stderr,
					"RAW---------------------------> %d %d %d\n",
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
						samp->tv.tv_sec = ev.input_event_sec;
						samp->tv.tv_usec = ev.input_event_usec;
						samp++;
						total++;
					}
					break;
				}
			} else {
				fprintf(stderr,
					"tslib: Unknown event type %d\n",
					ev.type);
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
	uint8_t pen_up = 0;
	static int32_t next_trackid;

	if (ts->fd != i->last_fd)
		i->last_fd = check_fd(i);

	if (i->last_fd == -1)
		return -ENODEV;

	if (i->buf == NULL || i->max_slots < max_slots || i->nr < nr) {
		if (i->buf) {
			for (j = 0; j < i->nr; j++)
				free(i->buf[j]);

			free(i->buf);
		}

		i->buf = malloc(nr * sizeof(struct ts_sample_mt *));
		if (!i->buf)
			return -ENOMEM;

		for (j = 0; j < nr; j++) {
			i->buf[j] = calloc(max_slots,
					   sizeof(struct ts_sample_mt));
			if (!i->buf[j])
				return -ENOMEM;
		}

		i->max_slots = max_slots;
		i->nr = nr;

		if (i->type_a) {
			i->last_pressure = calloc(max_slots, sizeof(int32_t));
			if (!i->last_pressure) {
				for (j = 0; j < nr; j++)
					free(i->buf[j]);

				return -ENOMEM;
			}
		}
	}

	if (i->no_pressure)
		set_pressure(i);

	for (j = 0; j < nr; j++) {
		for (k = 0; k < max_slots; k++) {
			i->buf[j][k].valid = 0;
			i->buf[j][k].pen_down = -1;
		}
	}

	while (total < nr) {
		memset(i->ev, 0, sizeof(i->ev));
		rd = read(ts->fd,
			  i->ev,
			  sizeof(struct input_event) * NUM_EVENTS_READ);
		if (rd == -1) {
			if (total == 0) {
				if (errno > 0)
					return errno * -1;
				else
					return errno;
			} else {
				return total;
			}
		} else if (rd < (int) sizeof(struct input_event)) {
			if (total == 0)
				return -1;
			else
				return total;
		}

		for (it = 0; it < rd / sizeof(struct input_event); it++) {
		#ifdef DEBUG
			printf("INPUT-RAW: read type %d  code %3d  value %4d  time %ld.%ld\n",
			       i->ev[it].type, i->ev[it].code,
			       i->ev[it].value, (long)i->ev[it].time.tv_sec,
			       (long)i->ev[it].time.tv_usec);
		#endif
			switch (i->ev[it].type) {
			case EV_KEY:
				switch (i->ev[it].code) {
				case BTN_TOUCH:
					i->buf[total][i->slot].pen_down = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					if (i->ev[it].value == 0)
						pen_up = 1;

					break;
				}
				break;
			case EV_SYN:
				switch (i->ev[it].code) {
				case SYN_REPORT:
					if (pen_up && i->no_pressure) {
						for (k = 0; k < max_slots; k++) {
							i->buf[total][k].pressure = 0;
						}
					}

					/* subtract last SYN_MT_REPORT to have the slot index */
					if (i->type_a && i->slot)
						i->slot--;

					if (i->slot >= max_slots) {
						fprintf(stderr, "Critical internal error\n");
						return -1;
					}

					if (i->type_a && i->slot < i->last_type_a_slots) {
						for (k = 0; k < max_slots; k++) {
							if (k < i->last_type_a_slots)
								continue;

							/* remember / generate other pen-ups */
							i->buf[total][k].pressure = 0;
							i->buf[total][k].tracking_id = -1;
							i->last_pressure[k] = 0;
							i->buf[total][k].valid |= TSLIB_MT_VALID;
						}
					}
					i->last_type_a_slots = i->slot;

					/* FIXME this pen_up is deprecated in MT */
					if (pen_up)
						pen_up = 0;

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
					if (!i->type_a)
						break;

					i->buf[total][i->slot].slot = i->slot;

					if (i->buf[total][i->slot].valid < 1) {
						/* SYN_MT_REPORT only is pen-up */
						i->buf[total][i->slot].pressure = 0;
						i->buf[total][i->slot].tracking_id = -1;
						i->last_pressure[i->slot] = 0;
					} else if (i->last_pressure[i->slot] == 0) {
						/* new contact. generate a tracking id */
						i->buf[total][i->slot].tracking_id = ++next_trackid;
						i->last_pressure[i->slot] = 1;
					}

					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;

					if (i->slot < max_slots)
						i->slot++;

					break;
			#ifdef DEBUG
				case SYN_DROPPED:
					fprintf(stderr,
						"INPUT-RAW: SYN_DROPPED\n");
					break;
			#endif
				}
				break;
			case EV_ABS:
				switch (i->ev[it].code) {
				case ABS_X:
					/* in case we didn't already get data for this
					 * slot, we go ahead and act as if this would be
					 * ABS_MT_POSITION_X
					 */
					if (i->mt && i->buf[total][i->slot].valid & TSLIB_MT_VALID)
						break;
					// fall through
				case ABS_MT_POSITION_X:
					i->buf[total][i->slot].x = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					break;
				case ABS_Y:
					if (i->mt && i->buf[total][i->slot].valid & TSLIB_MT_VALID)
						break;
					// fall through
				case ABS_MT_POSITION_Y:
					i->buf[total][i->slot].y = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					break;
				case ABS_PRESSURE:
					if (i->mt && i->buf[total][i->slot].valid & TSLIB_MT_VALID)
						break;
					// fall through
				case ABS_MT_PRESSURE:
					i->buf[total][i->slot].pressure = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					break;
				case ABS_MT_TOOL_X:
					i->buf[total][i->slot].tool_x = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					/* for future use
					 * i->buf[total][i->slot].valid |= TSLIB_MT_VALID_TOOL;
					 */
					break;
				case ABS_MT_TOOL_Y:
					i->buf[total][i->slot].tool_y = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					/* for future use
					 * i->buf[total][i->slot].valid |= TSLIB_MT_VALID_TOOL;
					 */
					break;
				case ABS_MT_TOOL_TYPE:
					i->buf[total][i->slot].tool_type = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					/* for future use
					 * i->buf[total][i->slot].valid |= TSLIB_MT_VALID_TOOL;
					 */
					break;
				case ABS_MT_ORIENTATION:
					i->buf[total][i->slot].orientation = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					break;
				case ABS_MT_DISTANCE:
					i->buf[total][i->slot].distance = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;

					if (i->special_device == EGALAX_VERSION_210) {
						if (i->ev[it].value > 0)
							i->buf[total][i->slot].pressure = 0;
						else
							i->buf[total][i->slot].pressure = 255;
					}

					break;
				case ABS_MT_BLOB_ID:
					i->buf[total][i->slot].blob_id = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					break;
				case ABS_MT_TOUCH_MAJOR:
					i->buf[total][i->slot].touch_major = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					if (i->ev[it].value == 0)
						i->buf[total][i->slot].pressure = 0;
					break;
				case ABS_MT_WIDTH_MAJOR:
					i->buf[total][i->slot].width_major = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					break;
				case ABS_MT_TOUCH_MINOR:
					i->buf[total][i->slot].touch_minor = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					break;
				case ABS_MT_WIDTH_MINOR:
					i->buf[total][i->slot].width_minor = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					break;
				case ABS_MT_TRACKING_ID:
					i->buf[total][i->slot].tracking_id = i->ev[it].value;
					i->buf[total][i->slot].tv.tv_sec = i->ev[it].input_event_sec;
					i->buf[total][i->slot].tv.tv_usec = i->ev[it].input_event_usec;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					if (i->ev[it].value == -1)
						i->buf[total][i->slot].pressure = 0;
					break;
				case ABS_MT_SLOT:
					if (i->ev[it].value < 0 || i->ev[it].value >= max_slots) {
						fprintf(stderr, "tslib: warning: slot out of range. data corrupted!\n");
						i->slot = max_slots - 1;
					} else {
						i->slot = i->ev[it].value;
						i->buf[total][i->slot].slot = i->ev[it].value;
						i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
					}
					break;
				}
				break;
			}
			if (total == ret)
				break;
		} /* just NUM_EVENTS_READ. it's simply 1 */
	}

	return total;
}

static int ts_input_fini(struct tslib_module_info *inf)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;
	int j;

	if (i->grab_events == GRAB_EVENTS_ACTIVE) {
		if (ioctl(ts->fd, EVIOCGRAB, (void *)0))
			fprintf(stderr, "tslib: Unable to un-grab selected input device\n");
	}

	for (j = 0; j < i->nr; j++)
		free(i->buf[j]);

	free(i->buf);
	free(i->last_pressure);

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

static const struct tslib_vars raw_vars[] = {
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
	i->special_device = 0;
	i->last_pressure = NULL;

	if (tslib_parse_vars(&i->module, raw_vars, NR_VARS, params)) {
		free(i);
		return NULL;
	}

	return &(i->module);
}

#ifndef TSLIB_STATIC_INPUT_MODULE
	TSLIB_MODULE_INIT(input_mod_init);
#endif
