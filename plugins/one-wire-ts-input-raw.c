/*
 * tslib/plugins/one-wire-ts-input-raw.c
 *
 * source: http://www.friendlyarm.net/forum/topic/3366
 * SPDX-License-Identifier: LGPL-2.1
 */
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <errno.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>

#include "config.h"
#include "tslib-private.h"

static int misc_read(struct tslib_module_info *inf, struct ts_sample *samp,
		     int nr)
{
	struct tsdev *ts = inf->dev;
	int ret, i;
	unsigned int ts_status;

	for (i = 0; i < nr; i++) {
		ret = read(ts->fd, &ts_status, sizeof ts_status);
		if (ret < 0)
			return i;

		if (!ret)
			return i;

		samp->x = ((ts_status) >> 16) & 0x7FFF;
		samp->y = ts_status & 0x7FFF;
		samp->pressure = ts_status >> 31;
		gettimeofday(&samp->tv,NULL);
		samp++;
	}

	return nr;
}

static int misc_read_mt(struct tslib_module_info *inf, struct ts_sample_mt **samp,
			int max_slots, int nr)
{
	struct tsdev *ts = inf->dev;
	int ret, i;
	unsigned int ts_status;

	if (max_slots > 1) {
	#ifdef DEBUG
		printf("only one slot supported\n");
	#endif
		max_slots = 1;
	}

	for (i = 0; i < nr; i++) {
		ret = read(ts->fd, &ts_status, sizeof ts_status);
		if (ret < 0)
			return i;

		if (!ret)
			return i;

		samp[i][0].x = ((ts_status) >> 16) & 0x7FFF;
		samp[i][0].y = ts_status & 0x7FFF;
		samp[i][0].pressure = ts_status >> 31;
		samp[i][0].valid |= TSLIB_MT_VALID;
		gettimeofday(&samp[i][0].tv, NULL);
	}

	return nr;
}

static int ts_fini(struct tslib_module_info *inf)
{
        free(inf);
        return 0;
}

static const struct tslib_ops misc_ops =
{
	.read		= misc_read,
	.read_mt	= misc_read_mt,
        .fini   	= ts_fini,
};

TSAPI struct tslib_module_info *one_wire_ts_input_mod_init(__attribute__ ((unused)) struct tsdev *dev,
							   __attribute__ ((unused)) const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &misc_ops;
	return m;
}

#ifndef TSLIB_STATIC_ONE_WIRE_TS_INPUT_MODULE
	TSLIB_MODULE_INIT(one_wire_ts_input_mod_init);
#endif
