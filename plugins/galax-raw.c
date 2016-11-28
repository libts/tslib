/*
 * tslib/plugins/galax-raw.c
 *
 * Inspired from input-raw.c
 * 2009.05 Fred Salabartan
 *
 * Add support for version 0100 by Oskari Rauta <oskari.rauta@gmail.com> 2013
 *
 * Modify by Jean-Baptiste Théou <jean-baptiste.theou@vodalys.com> (2013)
 * 
 * This file is placed under the LGPL. Please see the file
 * COPYING for more details.
 *
 * Plugin for "eGalax Inc. Touch" (using usbhid driver)
 * (Vendor=0eef Product=0001 Version=0112/210/0100)
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <limits.h>
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

#include "tslib-private.h"

#define GRAB_EVENTS_WANTED  1
#define GRAB_EVENTS_ACTIVE  2

#define BITS_PER_BYTE	8
#define BITS_PER_LONG	(sizeof(long) * BITS_PER_BYTE)

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BIT(nr) (1UL << (nr))
#define BIT_MASK(nr) (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr) ((nr) / BITS_PER_LONG)
#define BITS_TO_LONGS(nr) DIV_ROUND_UP(nr, BITS_PER_LONG)

#define VERSION_0112 1
#define VERSION_210  2
#define VERSION_0100 3
/* VERSION_210 must use /dev/input/event1 */


struct tslib_galax {
 struct tslib_module_info module;

 int current_x;
 int current_y;
 int current_p;
 int model_version;
 int sane_fd; 
 int grab_events;
};




static int ts_galax_check_fd (struct tslib_galax *i)
{
	struct tsdev *ts = i->module.dev;
	int version;
	long evbit[BITS_TO_LONGS(EV_CNT)];
	long absbit[BITS_TO_LONGS(ABS_CNT)];
	long keybit[BITS_TO_LONGS(KEY_CNT)];
	struct input_id infos;
	if (ioctl(ts->fd, EVIOCGVERSION, &version) < 0) {
		fprintf(stderr, "tslib: Selected device is not a Linux input event device\n");
		return -1;
	}

	if (version < EV_VERSION) {
		fprintf(stderr, "tslib: Selected device uses a different version of the event protocol than tslib was compiled for\n");
		return -1;
	}

 	if ((ioctl(ts->fd, EVIOCGID, &infos) < 0)) {
 		fprintf (stderr, "tslib: warning, can not read device identifier\n");
	} else if (infos.bustype != 3 || infos.vendor != 0x0EEF ||
		   (infos.product != 0x0001 && infos.product != 0x7200 && infos.product != 0x7201)) {
		fprintf (stderr, "tslib: this is not an eGalax touchscreen (3,0x0EEF,1/7200/7201,0x0112)\n"
			 "Your device: bus=%d, vendor=0x%X, product=0x%X, version=0x%X\n",
			 infos.bustype, infos.vendor, infos.product, infos.version);
 		return -1;
 	}
 	if(infos.version == 0x0112){
		i->model_version = VERSION_0112;
	}
	else if(infos.version == 0x210){
		i->model_version = VERSION_210;
	}
	else if(infos.version == 0x0100){
		i->model_version = VERSION_0100;
	}
	else{
		fprintf (stderr, "Unsupported model\n");
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
 	we set it to constant 255. It's still controlled by BTN_TOUCH - when not
 	touched, the pressure is forced to 0. */
	if (!(absbit[BIT_WORD(ABS_PRESSURE)] & BIT_MASK(ABS_PRESSURE))) {
		i->current_p = 255;

		if ((ioctl(ts->fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) ||
			!(keybit[BIT_WORD(BTN_TOUCH)] & BIT_MASK(BTN_TOUCH) ||
			  keybit[BIT_WORD(BTN_LEFT)] & BIT_MASK(BTN_LEFT))) {
			fprintf(stderr, "tslib: Selected device is not a touchscreen (must support BTN_TOUCH or BTN_LEFT events)\n");
			return -1;
		}
	}

 	if (!(evbit[BIT_WORD(EV_SYN)] & BIT_MASK(EV_SYN))) {
 		fprintf (stderr, "tslib: device does not use EV_SYN\n");
 		return -1;
 	}

 	if (i->grab_events == GRAB_EVENTS_WANTED) {
 		if (ioctl(ts->fd, EVIOCGRAB, (void *)1)) {
 			fprintf (stderr, "tslib: unable to grab selected input device\n");
 			return -1;
 		}
 		i->grab_events = GRAB_EVENTS_ACTIVE;
 	}

 return 0;
}



static int ts_galax_read (struct tslib_module_info *inf,
 					struct ts_sample *samp, int nr)
{
	struct tslib_galax *i = (struct tslib_galax *)inf;
	struct tsdev *ts = inf->dev;
	struct input_event ev;
	int ret = nr;
	int total = 0;
	if (i->sane_fd == 0)
 		i->sane_fd = ts_galax_check_fd(i);
 		
 	if (i->sane_fd == -1){
 		exit(0);
	}
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
						i->current_x = 0;
						i->current_y = 0;
						i->current_p = 0;
					} else {
						i->current_p = 255;
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
				samp++;
				total++;
				break;

			case EV_ABS:
				if(i->model_version == VERSION_0112){
					switch (ev.code) {
						case ABS_X+2: i->current_x = ev.value; break;
						case ABS_Y+2: i->current_y = ev.value; break;
						case ABS_PRESSURE: i->current_p = ev.value; break;
					}
				}
				else if(i->model_version == VERSION_210){
					switch (ev.code) {
						case ABS_X: i->current_x = ev.value; break;
						case ABS_Y: i->current_y = ev.value; break;
						case ABS_PRESSURE: i->current_p = ev.value; break;
					}
				}
				else if(i->model_version == VERSION_0100){
					switch (ev.code) {
						case ABS_X: i->current_x = ev.value; break;
						case ABS_Y: i->current_y = ev.value; break;
						case ABS_PRESSURE: i->current_p = ev.value; break;
					}
				}
				break;
		}
		ret = total;
	}
 	return ret;
}



static int ts_galax_fini (struct tslib_module_info *inf)
{
	struct tslib_galax *i = (struct tslib_galax *)inf;
	struct tsdev *ts = inf->dev;

	if (i->grab_events == GRAB_EVENTS_ACTIVE) {
		if (ioctl (ts->fd, EVIOCGRAB, (void *)0)) {
			fprintf (stderr, "tslib: Unable to un-grab selected input device\n");
 		}
	}

	free (inf);
 	return 0;
}



static const struct tslib_ops __ts_galax_ops = {
	.read = ts_galax_read,
	.fini = ts_galax_fini,
};



static int parse_raw_grab (struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_galax *i = (struct tslib_galax *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul (str, NULL, 0);

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


TSAPI struct tslib_module_info *galax_mod_init (__attribute__ ((unused)) struct tsdev *dev,
						const char *params)
{
 	struct tslib_galax *i;

 	i = malloc (sizeof (struct tslib_galax));
 	if (i == NULL)
 		return NULL;

 	i->module.ops = &__ts_galax_ops;
 	i->current_x = 0;
 	i->current_y = 0;
 	i->current_p = 0;
 	i->sane_fd = 0;
 	i->grab_events = 0;

 	if (tslib_parse_vars (&i->module, raw_vars, NR_VARS, params)) {
 		free (i);
 		return NULL;
	}

	return &(i->module);
}

#ifndef TSLIB_STATIC_GALAX_MODULE
	TSLIB_MODULE_INIT(galax_mod_init);
#endif 
