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
#include <stdint.h>

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
	int nr;
	int head;
	struct ts_hist hist[NR_SAMPHISTLEN];
	int *nr_mt;
	int *head_mt;
	struct ts_hist **hist_mt;
	int slots;
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

static void average_mt(struct tslib_dejitter *djt, struct ts_sample_mt **samp, int nr, int slot)
{
	const unsigned char *w;
	int sn = djt->head_mt[slot];
	int i, x = 0, y = 0;
	unsigned int p = 0;

        w = weight [djt->nr_mt[slot] - 2];

	for (i = 0; i < djt->nr_mt[slot]; i++) {
		x += djt->hist_mt[slot][sn].x * w [i];
		y += djt->hist_mt[slot][sn].y * w [i];
		p += djt->hist_mt[slot][sn].p * w [i];
		sn = (sn - 1) & (NR_SAMPHISTLEN - 1);
	}

	samp[nr][slot].x = x >> w [NR_SAMPHISTLEN];
	samp[nr][slot].y = y >> w [NR_SAMPHISTLEN];
	samp[nr][slot].pressure = p >> w [NR_SAMPHISTLEN];
#ifdef DEBUG
	fprintf(stderr,"DEJITTER----------------> %d %d %d\n",
		samp[nr][slot].x, samp[nr][slot].y, samp[nr][slot].pressure);
#endif
}

static int dejitter_read_mt(struct tslib_module_info *info, struct ts_sample_mt **samp, int max_slots, int nr)
{
        struct tslib_dejitter *djt = (struct tslib_dejitter *)info;
	int ret;
	int i, j;

	ret = info->next->ops->read_mt(info->next, samp, max_slots, nr);
	if (ret < 0)
		return ret;

#ifdef DEBUG
	if (ret == 0)
		fprintf (stderr, "DEJITTER: couldn't read data\n");

	printf("DEJITTER: read %d samples (mem: %d nr x %d slots)\n", ret, nr, max_slots);
#endif

	if (djt->hist_mt == NULL || max_slots > djt->slots) {
		if (djt->hist_mt) {
			for (i = 0; i < djt->slots; i++) {
				if (djt->hist_mt[i])
					free(djt->hist_mt[i]);
			}
			free(djt->hist_mt);
			djt->hist_mt = NULL;
		}

		djt->hist_mt = malloc(max_slots * sizeof(struct ts_hist *));
		if (!djt->hist_mt)
			return -ENOMEM;

		for (i = 0; i < max_slots; i++) {
			djt->hist_mt[i] = calloc(NR_SAMPHISTLEN, sizeof(struct ts_hist));
			if (&djt->hist[i] == NULL) {
				for (j = 0; j < i; j++)
					if (&djt->hist[j])
						free(&djt->hist[j]);

				if (djt->hist_mt)
					free(djt->hist_mt);

				return -ENOMEM;
			}
		}
		djt->slots = max_slots;
	}

	if (djt->nr_mt == NULL || max_slots > djt->slots) {
		if (djt->nr_mt)
			free(djt->nr_mt);

		djt->nr_mt = calloc(max_slots, sizeof(int));
		if (!djt->nr_mt)
			return -ENOMEM;
	}

	if (djt->head_mt == NULL || max_slots > djt->slots) {
		if (djt->head_mt)
			free(djt->head_mt);

		djt->head_mt = calloc(max_slots, sizeof(int));
		if (!djt->head_mt)
			return -ENOMEM;
	}

	for (j = 0; j < ret; j++) {
		for (i = 0; i < max_slots; i++) {
			if (samp[j][i].valid != 1)
				continue;

			if (samp[j][i].pressure == 0) {
				/*
				 * Pen was released. Reset the state and
				 * forget all history events.
				 */
				djt->nr_mt[i] = 0;
				continue;
			}

			/* If the pen moves too fast, reset the backlog. */
			if (djt->nr_mt[i]) {
				int prev = (djt->head_mt[i] - 1) & (NR_SAMPHISTLEN - 1);
				if (sqr(samp[j][i].x - djt->hist_mt[i][prev].x) +
				    sqr(samp[j][i].y - djt->hist_mt[i][prev].y) > djt->delta) {
	#ifdef DEBUG
					fprintf (stderr, "DEJITTER: pen movement exceeds threshold\n");
	#endif
					djt->nr_mt[i] = 0;
				}
			}

			djt->hist_mt[i][djt->head_mt[i]].x = samp[j][i].x;
			djt->hist_mt[i][djt->head_mt[i]].y = samp[j][i].y;
			djt->hist_mt[i][djt->head_mt[i]].p = samp[j][i].pressure;
			if (djt->nr_mt[i] < NR_SAMPHISTLEN)
				djt->nr_mt[i]++;

			/* We'll pass through the very first sample since
			 * we can't average it (no history yet).
			 */
			if (djt->nr_mt[i] > 1) {
				average_mt(djt, samp, j, i);
			}

			djt->head_mt[i] = (djt->head_mt[i] + 1) & (NR_SAMPHISTLEN - 1);
		}
	}

	return j;
}

static int dejitter_fini(struct tslib_module_info *info)
{
        struct tslib_dejitter *djt = (struct tslib_dejitter *)info;
	int i;

	for (i = 0; i < djt->slots; i++) {
		if (djt->hist_mt[i])
			free(djt->hist_mt[i]);
	}

	if (djt->hist_mt)
		free(djt->hist_mt);

	if (djt->nr_mt)
		free(djt->nr_mt);

	if (djt->head_mt)
		free(djt->head_mt);

	free(info);

	return 0;
}

static const struct tslib_ops dejitter_ops =
{
	.read		= dejitter_read,
	.read_mt	= dejitter_read_mt,
	.fini		= dejitter_fini,
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
	switch ((int)(intptr_t)data) {
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

TSAPI struct tslib_module_info *dejitter_mod_init(__attribute__ ((unused)) struct tsdev *dev,
						  const char *params)
{
	struct tslib_dejitter *djt;

	djt = malloc(sizeof(struct tslib_dejitter));
	if (djt == NULL)
		return NULL;

	memset(djt, 0, sizeof(struct tslib_dejitter));
	djt->module.ops = &dejitter_ops;

	djt->delta = 100;
        djt->head = 0;
        djt->hist_mt = NULL;
        djt->nr_mt = NULL;
        djt->head_mt = NULL;
        djt->slots = 0;

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
