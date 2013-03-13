/*
 * tslib/plugins/galax-raw.c
 *
 * Inspired from input-raw.c
 * 2009.05 Fred Salabartan
 *
 * This file is placed under the LGPL. Please see the file
 * COPYING for more details.
 *
 * Plugin for "eGalax Inc. Touch" (using usbhid driver)
 * (Vendor=0eef Product=0001 Version=0112)
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

#include "tslib-private.h"

enum {
 GRAB_EVENTS_NO = 0,
 GRAB_EVENTS_WANTED = 1,
 GRAB_EVENTS_ACTIVE = 2,

 SANE_INIT = 0,
 SANE_OK = 1,
 SANE_ERROR = 2,

 BITS_PER_BYTE = 8,
 BITS_PER_LONG = (sizeof(long) * BITS_PER_BYTE)
};

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BIT(nr) (1UL << (nr))
#define BIT_MASK(nr) (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr) ((nr) / BITS_PER_LONG)
#define BITS_TO_LONGS(nr) DIV_ROUND_UP(nr, BITS_PER_LONG)


struct tslib_galax {
 struct tslib_module_info module;

 int current_x;
 int current_y;
 int current_pressure;

 int sane_fd; // 0=not init, 1=ok, -1=error
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
 fprintf (stderr, "tslib: not a valid input device\n");
 return SANE_ERROR;
 }

 if (version != EV_VERSION) {
 fprintf (stderr, "tslib: bad event protocol version (lib vs device)\n");
 return SANE_ERROR;
 }

 if ((ioctl(ts->fd, EVIOCGID, &infos) < 0)) {
 fprintf (stderr, "tslib: warning, can not read device identifier\n");
 } else if (infos.bustype != 3 || infos.vendor != 0x0EEF || infos.product != 0x0001 || infos.version != 0x0112) {
 fprintf (stderr, "tslib: this is not an eGalax touchscreen (3,0x0EEF,1,0x0112)\n"
 "Your device: bus=%d, vendor=0x%X, product=0x%X, version=0x%X\n",
 infos.bustype, infos.vendor, infos.product, infos.version);
 return SANE_ERROR;
 }

 if ((ioctl(ts->fd, EVIOCGBIT(0, EV_CNT), evbit) < 0) ||
 !(evbit[BIT_WORD(EV_ABS)] & BIT_MASK(EV_ABS)) ||
 !(evbit[BIT_WORD(EV_KEY)] & BIT_MASK(EV_KEY)) ) {
 fprintf (stderr, "tslib: device does not support ABS and KEY event types\n");
 return SANE_ERROR;
 }

 if ((ioctl(ts->fd, EVIOCGBIT(EV_ABS, ABS_CNT), absbit)) < 0 ||
 !(absbit[BIT_WORD(ABS_X)] & BIT_MASK(ABS_X)) ||
 !(absbit[BIT_WORD(ABS_Y)] & BIT_MASK(ABS_Y))) {
 fprintf (stderr, "tslib: device does not support ABS_X and ABS_Y events\n");
 return SANE_ERROR;
 }

 /* Since some touchscreens (eg. infrared) physically can't measure pressure,
 the input system doesn't report it on those. Tslib relies on pressure, thus
 we set it to constant 255. It's still controlled by BTN_TOUCH - when not
 touched, the pressure is forced to 0. */
 if (!(absbit[BIT_WORD(ABS_PRESSURE)] & BIT_MASK(ABS_PRESSURE))) {
 i->current_pressure = 255;

 if ((ioctl(ts->fd, EVIOCGBIT(EV_KEY, KEY_CNT), keybit) < 0) ||
 !(keybit[BIT_WORD(BTN_TOUCH)] & BIT_MASK(BTN_TOUCH)) ) {
 fprintf (stderr, "tslib: device does support BTN_TOUCH events\n");
 return SANE_ERROR;
 }
 }

 if (! (evbit[BIT_WORD(EV_SYN)] & BIT_MASK(EV_SYN))) {
 fprintf (stderr, "tslib: device does not use EV_SYN\n");
 return SANE_ERROR;
 }

 if (i->grab_events == GRAB_EVENTS_WANTED) {
 if (ioctl(ts->fd, EVIOCGRAB, (void *)1)) {
 fprintf (stderr, "tslib: unable to grab selected input device\n");
 return -1;
 }
 i->grab_events = GRAB_EVENTS_ACTIVE;
 }

 return SANE_OK;
}



static int ts_galax_read (struct tslib_module_info *inf,
 struct ts_sample *samp, int nr)
{
 struct tslib_galax *i = (struct tslib_galax *)inf;
 struct tsdev *ts = inf->dev;
 struct input_event ev;
 int ret = nr;
 int total = 0;

 if (i->sane_fd == SANE_INIT)
 i->sane_fd = ts_galax_check_fd(i);

 if (i->sane_fd == SANE_ERROR)
 return 0;

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
 i->current_pressure = 0;
 } else {
 i->current_pressure = 255;
 }
 break;
 }
 break;

 case EV_SYN:
 /* Fill out a new complete event */
 samp->x = i->current_x;
 samp->y = i->current_y;
 samp->pressure = i->current_pressure;
 samp->tv = ev.time;
 samp++;
 total++;
 break;

 case EV_ABS:
 switch (ev.code) {
 case ABS_X+2: i->current_x = ev.value; break;
 case ABS_Y+2: i->current_y = ev.value; break;
 case ABS_PRESSURE: i->current_pressure = ev.value; break;
 }
 break;
 }
 }
 ret = total;

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



TSAPI struct tslib_module_info *mod_init (struct tsdev *dev, const char *params)
{
 struct tslib_galax *i;

 i = malloc (sizeof (struct tslib_galax));
 if (i == NULL)
 return NULL;

 i->module.ops = &__ts_galax_ops;
 i->current_x = 0;
 i->current_y = 0;
 i->current_pressure = 0;
 i->sane_fd = SANE_INIT;
 i->grab_events = GRAB_EVENTS_NO;

 if (tslib_parse_vars (&i->module, raw_vars, NR_VARS, params)) {
 free (i);
 return NULL;
 }

 return &(i->module);
}
