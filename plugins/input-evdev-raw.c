/*
 *  tslib/plugins/input-raw.c
 *
 *  Rewritten against libevdev
 *  Copyright (C) 2018 Martin Kepplinger <martin.kepplinger@ginzinger.com>
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
#include <fcntl.h>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sys/time.h>
#include <sys/types.h>

#include "tslib-private.h"

#include <libevdev/libevdev.h>

struct tslib_input {
	struct tslib_module_info module;

	int	current_x;
	int	current_y;
	int	current_p;

	int8_t	using_syn;
	int8_t	grab_events;

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

	struct libevdev *evdev;
	int8_t	fd_blocking;
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
#ifdef DEBUG
	printf("tslib input device: vendor 0x%X product 0x%X version 0x%X on bus 0x%X\n",
	       libevdev_get_id_vendor(i->evdev),
	       libevdev_get_id_product(i->evdev),
	       libevdev_get_id_version(i->evdev),
	       libevdev_get_id_bustype(i->evdev));
#endif

	/* only special usb devices so far, see the list above.
	 * so if not usb, we are done. */
	if (libevdev_get_id_bustype(i->evdev) != BUS_USB)
		return 0;

	switch (libevdev_get_id_vendor(i->evdev)) {
	case USB_VID_EGALAX:
		switch (libevdev_get_id_product(i->evdev)) {
		case 0x0001:
		/* taken from galax-raw. is this correct? */
		case 0x7200:
		case 0x7201:
			/* please note that any workarounds that don't
			 * apply to only a specific VERSION of one product,
			 * but to the product ID, should *really* be handled
			 * in the kernel! They have quirks over there.
			 */
			switch (libevdev_get_id_version(i->evdev)) {
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
	int rc;

	rc = fcntl(ts->fd, F_GETFL);
	if (rc == -1) {
		perror("fcntl");
		free(i);
		return -1;
	}
	if (rc & O_NONBLOCK) {
	#ifdef DEBUG
		printf("INPUT-RAW: opened non-blocking\n");
	#endif
		i->fd_blocking = 0;
	} else {
	#ifdef DEBUG
		printf("INPUT-RAW: opened blocking\n");
	#endif
		i->fd_blocking = 1;
	}

	/*
	 * TODO if last_fd != -2
	 *	libevdev_change_fd() ...
	 */

	rc = libevdev_new_from_fd(ts->fd, &i->evdev);
	if (rc < 0) {
		fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
		free(i);
		return -1;
	}


	if (libevdev_get_driver_version(i->evdev) < EV_VERSION) {
		fprintf(stderr,
			"tslib: Warning: device uses a different evdev version than tslib was compiled for\n");

	}

	if (!libevdev_has_event_type(i->evdev, EV_ABS)) {
		fprintf(stderr,
			"tslib: Selected device is not a touchscreen (must support ABS event type)\n");
		return -1;
	}

	if (!libevdev_has_event_code(i->evdev, EV_ABS, ABS_X) ||
	    !libevdev_has_event_code(i->evdev, EV_ABS, ABS_Y)) {
		if (!libevdev_has_event_code(i->evdev, EV_ABS, ABS_MT_POSITION_X) ||
		    !libevdev_has_event_code(i->evdev, EV_ABS, ABS_MT_POSITION_Y)) {
			fprintf(stderr,
				"tslib: Selected device is not a touchscreen (must support ABS_X/Y or ABS_MT_POSITION_X/Y events)\n");
			return -1;
		}
	}

	/* Remember whether we have a multitouch device. We need to know for ABS_X,
	 * ABS_Y and ABS_PRESSURE data.
	 */
	if (libevdev_has_event_code(i->evdev, EV_ABS, ABS_MT_POSITION_X) &&
	    libevdev_has_event_code(i->evdev, EV_ABS, ABS_MT_POSITION_Y))
		i->mt = 1;

	if (libevdev_has_event_type(i->evdev, EV_KEY)) {
		/* for multitouch type B, tracking id is enough for pen down/up. type A
		 * has pen down/up through the list of (empty) SYN_MT_REPORT
		 * only for singletouch we need BTN_TOUCH or BTN_LEFT
		 */
		if (!(libevdev_has_event_code(i->evdev, EV_KEY, BTN_TOUCH) ||
		      libevdev_has_event_code(i->evdev, EV_KEY, BTN_LEFT)) && !i->mt) {
			fprintf(stderr,
				"tslib: Selected device is not a touchscreen (missing BTN_TOUCH or BTN_LEFT)\n");
			return -1;
		}
	}
	if (!libevdev_has_event_type(i->evdev, EV_SYN)) {
		i->using_syn = 0;
	#ifdef DEBUG
		printf("INPUT-RAW: device without EV_SYN\n");
	#endif
	}

	/* read device info and set special device nr */
	get_special_device(i);

	/* Since some touchscreens (eg. infrared) physically can't measure pressure,
	 * the input system doesn't report it on those. Tslib relies on pressure, thus
	 * we set it to constant 255. It's still controlled by BTN_TOUCH/BTN_LEFT -
	 * when not touched, the pressure is forced to 0.
	 */
	if (!libevdev_has_event_code(i->evdev, EV_ABS, ABS_PRESSURE))
		i->no_pressure = 1;

	if (i->mt) {
		if (!libevdev_has_event_code(i->evdev, EV_ABS, ABS_MT_PRESSURE))
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

	if (i->mt &&
	    !libevdev_has_event_code(i->evdev, EV_ABS, ABS_MT_SLOT) &&
	    !libevdev_has_event_code(i->evdev, EV_ABS, ABS_MT_TRACKING_ID)) {
		i->type_a = 1;
	#ifdef DEBUG
		printf("tslib: We have a multitouch type A device\n");
	#endif
	}

	if (i->mt && !i->using_syn) {
		fprintf(stderr,
			"tslib: Unsupported multitouch device (missing EV_SYN)\n");
		return -1;
	}

	if (libevdev_grab(i->evdev, i->grab_events) < 0) {
		fprintf(stderr,
			"tslib: Unable to grab selected input device\n");
		return -1;
	}

	return ts->fd;
}

static int ts_input_read_without_syn(struct tslib_module_info *inf,
				     struct ts_sample *samp, int nr)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;
	int len = sizeof(struct input_event);
	int ret, total = 0;
	struct input_event ev;
	unsigned char *p = (unsigned char *) &ev;

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
			samp->tv = ev.time;
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
					samp->tv = ev.time;
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
	return total;
}

static int ts_input_read(struct tslib_module_info *inf,
			 struct ts_sample *samp, int nr)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;
	struct input_event ev;
	int ret = nr;
	int rc;
	int total = 0;
	int pen_up = 0;

	if (ts->fd != i->last_fd)
		i->last_fd = check_fd(i);

	if (i->last_fd == -1)
		return -ENODEV;

	if (i->no_pressure)
		set_pressure(i);

	if (!i->using_syn)
		return ts_input_read_without_syn(inf, samp, nr);

	while (total < nr) {
		if (i->fd_blocking == 1) {
			rc = libevdev_next_event(i->evdev,
						LIBEVDEV_READ_FLAG_NORMAL|
						LIBEVDEV_READ_FLAG_BLOCKING,
						&ev);
		} else if (i->fd_blocking == 0) {
			rc = libevdev_next_event(i->evdev,
						LIBEVDEV_READ_FLAG_NORMAL,
						&ev);
		} else {
			fprintf(stderr, "Error initializing input module\n");
			return -EINVAL;
		}

		if (rc == LIBEVDEV_READ_STATUS_SYNC) {
		#ifdef DEBUG
			printf("INPUT-RAW: Frame dropped\n");
		#endif
			while (rc == LIBEVDEV_READ_STATUS_SYNC) {
				rc = libevdev_next_event(i->evdev,
							LIBEVDEV_READ_FLAG_SYNC,
							&ev);
			}
		#ifdef DEBUG
			printf("INPUT-RAW: re-synced\n");
		#endif
		} else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
		#ifdef DEBUG
			printf("INPUT-RAW: normal event read\n");
		#endif
		} else if (rc == -EAGAIN) {
			if (total > 0)
				return total;
			else
				return rc;
		} else {
			fprintf(stderr, "Failed to handle events: %s\n",
				strerror(-rc));
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
			fprintf(stderr,
				"RAW nr %d ---------------------> %d %d %d %ld.%ld\n",
				total,
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

	return ret;
}

static int ts_input_read_mt(struct tslib_module_info *inf,
			    struct ts_sample_mt **samp, int max_slots, int nr)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;
	int rc;
	int total = 0;
	int j, k;
	uint8_t pen_up = 0;
	static int32_t next_trackid;
	struct input_event ev;

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
		if (i->fd_blocking == 1) {
			rc = libevdev_next_event(i->evdev,
						LIBEVDEV_READ_FLAG_NORMAL|
						LIBEVDEV_READ_FLAG_BLOCKING,
						&ev);
		} else if (i->fd_blocking == 0) {
			rc = libevdev_next_event(i->evdev,
						LIBEVDEV_READ_FLAG_NORMAL,
						&ev);
		} else {
			fprintf(stderr, "Error initializing input module\n");
			return -EINVAL;
		}

		if (rc == LIBEVDEV_READ_STATUS_SYNC) {
		#ifdef DEBUG
			printf("INPUT-RAW: Frame dropped\n");
		#endif
			while (rc == LIBEVDEV_READ_STATUS_SYNC) {
				rc = libevdev_next_event(i->evdev,
							LIBEVDEV_READ_FLAG_SYNC,
							&ev);
			}
		#ifdef DEBUG
			printf("INPUT-RAW: re-synced\n");
		#endif
		} else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
		#ifdef DEBUG
			printf("INPUT-RAW: normal event read\n");
		#endif
		} else if (rc == -EAGAIN) {
			if (total > 0)
				return total;
			else
				return rc;
		} else {
			fprintf(stderr, "Failed to handle events: %s\n",
				strerror(-rc));
		}

	#ifdef DEBUG
		printf("INPUT-RAW nr %d: read type %d  code %3d  value %4d  time %ld.%ld\n",
		       total,
		       ev.type, ev.code,
		       ev.value, (long)ev.time.tv_sec,
		       (long)ev.time.tv_usec);
	#endif
		switch (ev.type) {
		case EV_KEY:
			switch (ev.code) {
			case BTN_TOUCH:
				i->buf[total][i->slot].pen_down = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				if (ev.value == 0)
					pen_up = 1;

				break;
			}
			break;
		case EV_SYN:
			switch (ev.code) {
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
			}
			break;
		case EV_ABS:
			switch (ev.code) {
			case ABS_X:
				/* in case we didn't already get data for this
				 * slot, we go ahead and act as if this would be
				 * ABS_MT_POSITION_X
				 */
				if (i->mt && i->buf[total][i->slot].valid & TSLIB_MT_VALID)
					break;
				// fall through
			case ABS_MT_POSITION_X:
				i->buf[total][i->slot].x = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				break;
			case ABS_Y:
				if (i->mt && i->buf[total][i->slot].valid & TSLIB_MT_VALID)
					break;
				// fall through
			case ABS_MT_POSITION_Y:
				i->buf[total][i->slot].y = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				break;
			case ABS_PRESSURE:
				if (i->mt && i->buf[total][i->slot].valid & TSLIB_MT_VALID)
					break;
				// fall through
			case ABS_MT_PRESSURE:
				i->buf[total][i->slot].pressure = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				break;
			case ABS_MT_TOOL_X:
				i->buf[total][i->slot].tool_x = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				/* for future use
				 * i->buf[total][i->slot].valid |= TSLIB_MT_VALID_TOOL;
				 */
				break;
			case ABS_MT_TOOL_Y:
				i->buf[total][i->slot].tool_y = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				/* for future use
				 * i->buf[total][i->slot].valid |= TSLIB_MT_VALID_TOOL;
				 */
				break;
			case ABS_MT_TOOL_TYPE:
				i->buf[total][i->slot].tool_type = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				/* for future use
				 * i->buf[total][i->slot].valid |= TSLIB_MT_VALID_TOOL;
				 */
				break;
			case ABS_MT_ORIENTATION:
				i->buf[total][i->slot].orientation = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				break;
			case ABS_MT_DISTANCE:
				i->buf[total][i->slot].distance = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;

				if (i->special_device == EGALAX_VERSION_210) {
					if (ev.value > 0)
						i->buf[total][i->slot].pressure = 0;
					else
						i->buf[total][i->slot].pressure = 255;
				}

				break;
			case ABS_MT_BLOB_ID:
				i->buf[total][i->slot].blob_id = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				break;
			case ABS_MT_TOUCH_MAJOR:
				i->buf[total][i->slot].touch_major = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				if (ev.value == 0)
					i->buf[total][i->slot].pressure = 0;
				break;
			case ABS_MT_WIDTH_MAJOR:
				i->buf[total][i->slot].width_major = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				break;
			case ABS_MT_TOUCH_MINOR:
				i->buf[total][i->slot].touch_minor = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				break;
			case ABS_MT_WIDTH_MINOR:
				i->buf[total][i->slot].width_minor = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				break;
			case ABS_MT_TRACKING_ID:
				i->buf[total][i->slot].tracking_id = ev.value;
				i->buf[total][i->slot].tv = ev.time;
				i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				if (ev.value == -1)
					i->buf[total][i->slot].pressure = 0;
				break;
			case ABS_MT_SLOT:
				if (ev.value < 0 || ev.value >= max_slots) {
					fprintf(stderr, "tslib: warning: slot out of range. data corrupted!\n");
					i->slot = max_slots - 1;
				} else {
					i->slot = ev.value;
					i->buf[total][i->slot].slot = ev.value;
					i->buf[total][i->slot].valid |= TSLIB_MT_VALID;
				}
				break;
			}
			break;
		}
		if (total == nr)
			break;
	}

	return total;
}

static int ts_input_fini(struct tslib_module_info *inf)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	int j;

	if (libevdev_grab(i->evdev, LIBEVDEV_UNGRAB) < 0)
		fprintf(stderr, "tslib: Unable to un-grab selected input device\n");

	libevdev_free(i->evdev);

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
			i->grab_events = LIBEVDEV_GRAB;
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

TSAPI struct tslib_module_info *input_evdev_mod_init(__attribute__ ((unused)) struct tsdev *dev,
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
	i->grab_events = LIBEVDEV_UNGRAB;
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
	i->fd_blocking = -1;
	i->evdev = NULL;
	i->using_syn = 1;

	if (tslib_parse_vars(&i->module, raw_vars, NR_VARS, params)) {
		free(i);
		return NULL;
	}

	return &i->module;
}

#ifndef TSLIB_STATIC_INPUT_EVDEV_MODULE
	TSLIB_MODULE_INIT(input_evdev_mod_init);
#endif
