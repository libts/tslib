/*
 * tslib driver for WaveShare touchscreens
 * Copyright (C) 2015 Peter Vicman
 * inspiration from derekhe:
 * https://github.com/derekhe/wavesahre-7inch-touchscreen-driver
 *
 * This file is placed under the LGPL.  Please see the file COPYING for more
 * details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Usage:
 *   module_raw waveshare vid_pid=<vendorID>:<productID> len=<raw_data_len>
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
#include <stdint.h>

#include "config.h"
#include "tslib-private.h"

struct tslib_input {
	struct tslib_module_info module;
	int vendor;
	int product;
	int len;
};

static int waveshare_read(struct tslib_module_info *inf, struct ts_sample *samp,
			  int nr)
{
	static short reopen = 1;
	struct stat devstat;
	struct hidraw_devinfo info;
	char name_buf[512];
	int cnt;
	short found = 0;
	struct tslib_input *i = (struct tslib_input *) inf;
	struct tsdev *ts = inf->dev;
	struct tsdev *ts_tmp;
	char *buf;
	int ret;

	if (reopen == 1) {
		reopen = 0;

		if (i->vendor > 0 && i->product > 0) {
#ifdef DEBUG
			fprintf(stderr,
				"waveshare: searching for device using hidraw...\n");
#endif
			for (cnt = 0; cnt < HIDRAW_MAX_DEVICES; cnt++) {
				snprintf(name_buf, sizeof(name_buf),
					 "/dev/hidraw%d", cnt);
#ifdef DEBUG
				fprintf(stderr,
					"waveshare: device: %s\n", name_buf);
#endif
				ret = stat(name_buf, &devstat);
				if (ret < 0)
					continue;

				ts_tmp = ts_open(name_buf, 0);
				if (!ts_tmp)
					continue;

#ifdef DEBUG
				fprintf(stderr, "  opened\n");
#endif
				ret = ioctl(ts_tmp->fd, HIDIOCGRAWINFO, &info);
				if (ret < 0) {
					ts_close(ts_tmp);
					continue;
				}

				info.vendor &= 0xFFFF;
				info.product &= 0xFFFF;
#ifdef DEBUG
				fprintf(stderr,
					"  vid=%04X, pid=%04X\n",
					info.vendor, info.product);
#endif

				if (i->vendor == info.vendor &&
				    i->product == info.product) {
					close(ts->fd);

					ts->fd = ts_tmp->fd;
					found = 1;
#ifdef DEBUG
					fprintf(stderr, "  correct device\n");
#endif
					ts_close(ts_tmp);
					break;
				}

				ts_close(ts_tmp);
			} /* for HIDRAW_MAX_DEVICES */

			if (found == 0)
				return -1;
		} /* vid/pid set */
	} /* reopen */

	buf = alloca(i->len * nr);

	ret = read(ts->fd, buf, (size_t) i->len * nr);
	if (ret > 0) {
		while (ret >= (int) i->len) {
	/*
	  0000271: aa01 00e4 0139 bb01 01e0 0320 01e0 0320 01e0 0320 01e0 0320 cc  .....9..... ... ... ... .

	  "aa" is start of the command,
	  "01" means clicked, while
	  "00" means unclicked.
	  "00e4" and "0139" is the X,Y position (HEX).
	  "bb" is start of multi-touch,
	  and the following bytes are the position of each point.
	 */
			samp->pressure = buf[1] & 0xff;
			samp->x = ((buf[2] & 0xff) << 8) | (buf[3] & 0xff);
			samp->y = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
			gettimeofday(&samp->tv, NULL);
		#ifdef DEBUG
			fprintf(stderr, "waveshare raw: %d %d %d\n",
				samp->x, samp->y, samp->pressure);
			fprintf(stderr, "%x %x %x %x %x %x\n",
				buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
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

static int waveshare_read_mt(struct tslib_module_info *inf,
			     struct ts_sample_mt **samp, int max_slots, int nr)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;
	static short reopen = 1;
	struct stat devstat;
	struct hidraw_devinfo info;
	char name_buf[512];
	int cnt;
	short found = 0;
	struct tsdev *ts_tmp;
	char *buf;
	int ret;
	int count = 0;

	if (max_slots > 1) {
	#ifdef DEBUG
		printf("WAVESHARE: only one slot supported\n");
	#endif
		max_slots = 1;
	}

	if (reopen == 1) {
		reopen = 0;

		if (i->vendor > 0 && i->product > 0) {
#ifdef DEBUG
			fprintf(stderr,
				"waveshare: searching for device using hidraw...\n");
#endif
			for (cnt = 0; cnt < HIDRAW_MAX_DEVICES; cnt++) {
				snprintf(name_buf, sizeof(name_buf),
					 "/dev/hidraw%d", cnt);
#ifdef DEBUG
				fprintf(stderr,
					"waveshare: device: %s\n", name_buf);
#endif
				ret = stat(name_buf, &devstat);
				if (ret < 0)
					continue;

				ts_tmp = ts_open(name_buf, 0);
				if (!ts_tmp)
					continue;

#ifdef DEBUG
				fprintf(stderr, "  opened\n");
#endif
				ret = ioctl(ts_tmp->fd, HIDIOCGRAWINFO, &info);
				if (ret < 0) {
					ts_close(ts_tmp);
					continue;
				}

				info.vendor &= 0xFFFF;
				info.product &= 0xFFFF;
#ifdef DEBUG
				fprintf(stderr,
					"  vid=%04X, pid=%04X\n",
					info.vendor, info.product);
#endif

				if (i->vendor == info.vendor &&
				    i->product == info.product) {
					close(ts->fd);

					ts->fd = ts_tmp->fd;
					found = 1;
#ifdef DEBUG
					fprintf(stderr, "  correct device\n");
#endif
					ts_close(ts_tmp);
					break;
				}

				ts_close(ts_tmp);
			} /* for HIDRAW_MAX_DEVICES */

			if (found == 0)
				return -1;
		} /* vid/pid set */
	} /* reopen */

	buf = alloca(i->len * nr);

	ret = read(ts->fd, buf, (size_t) i->len * nr);
	if (ret > 0) {
		while (ret >= (int) i->len) {
	/*
	  0000271: aa01 00e4 0139 bb01 01e0 0320 01e0 0320 01e0 0320 01e0 0320 cc  .....9..... ... ... ... .

	  "aa" is start of the command,
	  "01" means clicked, while
	  "00" means unclicked.
	  "00e4" and "0139" is the X,Y position (HEX).
	  "bb" is start of multi-touch,
	  and the following bytes are the position of each point.
	 */
			samp[count][0].pressure = buf[1] & 0xff;
			samp[count][0].x = ((buf[2] & 0xff) << 8) | (buf[3] & 0xff);
			samp[count][0].y = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
			samp[count][0].valid |= TSLIB_MT_VALID;
			gettimeofday(&samp[count][0].tv, NULL);
		#ifdef DEBUG
			fprintf(stderr, "waveshare raw: %d %d %d\n",
				samp[count][0].x, samp[count][0].y, samp[count][0].pressure);
			fprintf(stderr, "%x %x %x %x %x %x\n",
				buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		#endif
			count++;
			buf += i->len;
			ret -= i->len;
		}
	} else {
		return -1;
	}

	return nr;
}

static const struct tslib_ops waveshare_ops = {
	.read = waveshare_read,
	.read_mt = waveshare_read_mt,
};

static int parse_vid_pid(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_input *i = (struct tslib_input *)inf;

	if (strlen(str) < 9 || (int)(intptr_t) data != 1)
		return 0;   /* -1 */

	str[4] = str[9] = '\0';
	i->vendor = strtol(&str[0], NULL, 16);
	i->product = strtol(&str[5], NULL, 16);
#ifdef DEBUG
	fprintf(stderr, "waveshare vid:pid - %04X:%04X\n",
		i->vendor, i->product);
#endif /*DEBUG*/
	return 0;
}

static int parse_len(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	int v;
	int err = errno;

	v = atoi(str);

	if (v < 0)
		return -1;

	errno = err;
	switch ((int)(intptr_t) data) {
	case 1:
		i->len = v;
#ifdef DEBUG
		fprintf(stderr, "waveshare raw data len: %d bytes\n", i->len);
#endif
		break;
	default:
		return -1;
	}
	return 0;
}

static const struct tslib_vars raw_vars[] = {
	{ "vid_pid", (void *) 1, parse_vid_pid },
	{ "len", (void *) 1, parse_len },
};

#define NR_VARS (sizeof(raw_vars) / sizeof(raw_vars[0]))

TSAPI struct tslib_module_info *waveshare_mod_init(struct tsdev *dev,
						   const char *params)
{
	struct tslib_input *i;

	(void) dev;

	i = malloc(sizeof(struct tslib_input));
	if (i == NULL)
		return NULL;

	i->module.ops = &waveshare_ops;
	i->vendor = 0;
	i->product = 0;
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
