/*
 * tslib driver for WaveShare touchscreens
 * Copyright (C) 2015 Peter Vicman
 * inspiration from derekhe: https://github.com/derekhe/wavesahre-7inch-touchscreen-driver
 *
 * This file is placed under the LGPL.  Please see the file COPYING for more
 * details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <stdbool.h>

#include "config.h"
#include "tslib-private.h"

struct tslib_input {
  struct tslib_module_info module;
  int len;
};

static int waveshare_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
  struct tslib_input *i = (struct tslib_input *) inf;
  struct tsdev *ts = inf->dev;
  char *buf;
  int ret;

  buf = alloca(i->len * nr);

  ret = read(ts->fd, buf, i->len * nr);
  if(ret > 0) {
    while(ret >= (int) i->len) {
      /*
        0000271: aa01 00e4 0139 bb01 01e0 0320 01e0 0320 01e0 0320 01e0 0320 cc  .....9..... ... ... ... .

        "aa" is start of the command, "01" means clicked while "00" means unclicked.
        "00e4" and "0139" is the X,Y position (HEX).
        "bb" is start of multi-touch, and the following bytes are the position of each point.
      */
      samp->pressure = buf[1] & 0xff;
      samp->x = ((buf[2] & 0xff) << 8) | (buf[3] & 0xff);
      samp->y = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
      gettimeofday(&samp->tv, NULL);
#ifdef DEBUG
        fprintf(stderr, "waveshare raw: %d %d %d\n", samp->x, samp->y, samp->pressure);
        fprintf(stderr, "%x %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
#endif
      samp++;
      buf += i->len;
      ret -= i->len;
    }
  } else {
    return -1;
  }

  return nr;
}

static const struct tslib_ops waveshare_ops =
{
  .read = waveshare_read,
};

static int parse_len(struct tslib_module_info *inf, char *str, void *data)
{
  struct tslib_input *i = (struct tslib_input *)inf;
  int v;
  int err = errno;

  v = atoi(str);

  if (v < 0)
    return -1;

  errno = err;
  switch ((int) data) {
    case 1:
      i->len = v;
      fprintf(stderr, "waveshare raw data len: %d bytes\n", i->len);
      break;
    default:
      return -1;
  }
  return 0;
}

static const struct tslib_vars raw_vars[] =
{
  { "len", (void *) 1, parse_len },
};

#define NR_VARS (sizeof(raw_vars) / sizeof(raw_vars[0]))

TSAPI struct tslib_module_info *waveshare_mod_init(struct tsdev *dev, const char *params)
{
  struct tslib_input *i;

  (void) dev;

  i = malloc(sizeof(struct tslib_input));
  if (i == NULL)
    return NULL;

  i->module.ops = &waveshare_ops;
  i->len = 25;

  if (tslib_parse_vars(&i->module, raw_vars, NR_VARS, params)) {
    free(i);
    return NULL;
  }

  return &(i->module);
}

#ifndef TSLIB_STATIC_WAVESHARE_MODULE
  TSLIB_MODULE_INIT(waveshare_mod_init);
#endif
