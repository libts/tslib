/*
 *  tslib/plugins/dejitter.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Problem: some touchscreens read the X/Y values from ADC with a
 * great level of noise in their lowest bits. This produces "jitter"
 * in touchscreen output, e.g. even if we hold the stylus still,
 * we get a great deal of X/Y coordinate pairs that are close enough
 * but not equal. Also if we try to draw a straight line in a painter
 * program, we'll get a line full of spikes.
 *
 * Solution: we apply a smoothing filter on the last several values
 * thus excluding spikes from output. If we detect a substantial change
 * in coordinates, we reset the backlog of pen positions, thus avoiding
 * smoothing coordinates that are not supposed to be smoothed. This
 * supposes all noise has been filtered by the lower-level filter,
 * e.g. by the "variance" module.
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <stdio.h>

#include "config.h"
#include "tslib.h"
#include "tslib-filter.h"

/**
 * This filter works as follows: we keep track of latest N samples,
 * and average them with certain weights. The oldest samples have the
 * least weight and the most recent samples have the most weight.
 * This helps remove the jitter and at the same time doesn't influence
 * responsivity because for each input sample we generate one output
 * sample; pen movement becomes just somehow more smooth.
 */

#define NR_SAMPHISTLEN	4

/* To keep things simple (avoiding division) we ensure that
 * SUM(weight) = power-of-two. Also we must know how to approximate
 * measurements when we have less than NR_SAMPHISTLEN samples.
 */
static const unsigned char weight [NR_SAMPHISTLEN - 1][NR_SAMPHISTLEN + 1] =
{
	/* The last element is pow2(SUM(0..3)) */
	{ 5, 3, 0, 0, 3 },	/* When we have 2 samples ... */
	{ 8, 5, 3, 0, 4 },	/* When we have 3 samples ... */
	{ 6, 4, 3, 3, 4 },	/* When we have 4 samples ... */
};

struct ts_hist {
	int x;
	int y;
	unsigned int p;
};

struct tslib_dejitter {
	struct tslib_module_info module;
	int delta;
	int x;
	int y;
	int down;
	int nr;
	int head;
	struct ts_hist hist[NR_SAMPHISTLEN];
};

static int sqr (int x)
{
	return x * x;
}

static void average (struct tslib_dejitter *djt, struct ts_sample *samp)
{
	const unsigned char *w;
	int sn = djt->head;
	int i, x = 0, y = 0;
	unsigned int p = 0;

        w = weight [djt->nr - 2];

	for (i = 0; i < djt->nr; i++) {
		x += djt->hist [sn].x * w [i];
		y += djt->hist [sn].y * w [i];
		p += djt->hist [sn].p * w [i];
		sn = (sn - 1) & (NR_SAMPHISTLEN - 1);
	}

	samp->x = x >> w [NR_SAMPHISTLEN];
	samp->y = y >> w [NR_SAMPHISTLEN];
	samp->pressure = p >> w [NR_SAMPHISTLEN];
#ifdef DEBUG
	fprintf(stderr,"DEJITTER----------------> %d %d %d\n",
		samp->x, samp->y, samp->pressure);
#endif
}

static int dejitter_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
        struct tslib_dejitter *djt = (struct tslib_dejitter *)info;
	struct ts_sample *s;
	int count = 0, ret;

	ret = info->next->ops->read(info->next, samp, nr);
	for (s = samp; ret > 0; s++, ret--) {
		if (s->pressure == 0) {
			/*
			 * Pen was released. Reset the state and
			 * forget all history events.
			 */
			djt->nr = 0;
			samp [count++] = *s;
                        continue;
		}

                /* If the pen moves too fast, reset the backlog. */
		if (djt->nr) {
			int prev = (djt->head - 1) & (NR_SAMPHISTLEN - 1);
			if (sqr (s->x - djt->hist [prev].x) +
			    sqr (s->y - djt->hist [prev].y) > djt->delta) {
#ifdef DEBUG
				fprintf (stderr, "DEJITTER: pen movement exceeds threshold\n");
#endif
                                djt->nr = 0;
			}
		}

		djt->hist[djt->head].x = s->x;
		djt->hist[djt->head].y = s->y;
		djt->hist[djt->head].p = s->pressure;
		if (djt->nr < NR_SAMPHISTLEN)
			djt->nr++;

		/* We'll pass through the very first sample since
		 * we can't average it (no history yet).
		 */
		if (djt->nr == 1)
			samp [count] = *s;
		else {
			average (djt, samp + count);
			samp [count].tv = s->tv;
		}
		count++;

		djt->head = (djt->head + 1) & (NR_SAMPHISTLEN - 1);
	}

	return count;
}

static int dejitter_fini(struct tslib_module_info *info)
{
	free(info);
	return 0;
}

static const struct tslib_ops dejitter_ops =
{
	.read	= dejitter_read,
	.fini	= dejitter_fini,
};

static int dejitter_limit(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_dejitter *djt = (struct tslib_dejitter *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul(str, NULL, 0);

	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)data) {
	case 1:
		djt->delta = v;
		break;

	default:
		return -1;
	}
	return 0;
}

static const struct tslib_vars dejitter_vars[] =
{
	{ "delta",	(void *)1, dejitter_limit },
};

#define NR_VARS (sizeof(dejitter_vars) / sizeof(dejitter_vars[0]))

TSAPI struct tslib_module_info *dejitter_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_dejitter *djt;

	djt = malloc(sizeof(struct tslib_dejitter));
	if (djt == NULL)
		return NULL;

	memset(djt, 0, sizeof(struct tslib_dejitter));
	djt->module.ops = &dejitter_ops;

	djt->delta = 100;
        djt->head = 0;

	if (tslib_parse_vars(&djt->module, dejitter_vars, NR_VARS, params)) {
		free(djt);
		return NULL;
	}
	djt->delta = sqr (djt->delta);

	return &djt->module;
}

#ifndef TSLIB_STATIC_DEJITTER_MODULE
	TSLIB_MODULE_INIT(dejitter_mod_init);
#endif
