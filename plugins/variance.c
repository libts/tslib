/*
 *  tslib/plugins/variance.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: variance.c,v 1.6 2005/02/26 01:47:23 kergoth Exp $
 *
 * Variance filter for touchscreen values.
 *
 * Problem: some touchscreens are sampled very roughly, thus even if
 * you hold the pen still, the samples can differ, sometimes substantially.
 * The worst happens when electric noise during sampling causes the result
 * to be substantially different from the real pen position; this causes
 * the mouse cursor to suddenly "jump" and then return back.
 *
 * Solution: delay sampled data by one timeslot. If we see that the last
 * sample read differs too much, we mark it as "suspicious". If next sample
 * read is close to the sample before the "suspicious", the suspicious sample
 * is dropped, otherwise we consider that a quick pen movement is in progress
 * and pass through both the "suspicious" sample and the sample after it.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "tslib.h"
#include "tslib-filter.h"

struct tslib_variance {
	struct tslib_module_info module;
	int delta;
        struct ts_sample last;
        struct ts_sample noise;
	unsigned int flags;
#define VAR_PENDOWN		0x00000001
#define VAR_LASTVALID		0x00000002
#define VAR_NOISEVALID		0x00000004
#define VAR_SUBMITNOISE		0x00000008
};

static int sqr (int x)
{
	return x * x;
}

static int variance_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_variance *var = (struct tslib_variance *)info;
	struct ts_sample cur;
	int count = 0, dist;

	while (count < nr) {
		if (var->flags & VAR_SUBMITNOISE) {
			cur = var->noise;
			var->flags &= ~VAR_SUBMITNOISE;
		} else {
			if (info->next->ops->read(info->next, &cur, 1) < 1)
				return count;
		}

		if (cur.pressure == 0) {
			/* Flush the queue immediately when the pen is just
			 * released, otherwise the previous layer will
			 * get the pen up notification too late. This 
			 * will happen if info->next->ops->read() blocks.
			 */
			if (var->flags & VAR_PENDOWN) {
				var->flags |= VAR_SUBMITNOISE;
				var->noise = cur;
			}
			/* Reset the state machine on pen up events. */
			var->flags &= ~(VAR_PENDOWN | VAR_NOISEVALID | VAR_LASTVALID);
			goto acceptsample;
		} else
			var->flags |= VAR_PENDOWN;

		if (!(var->flags & VAR_LASTVALID)) {
			var->last = cur;
			var->flags |= VAR_LASTVALID;
			continue;
		}

		if (var->flags & VAR_PENDOWN) {
			/* Compute the distance between last sample and current */
			dist = sqr (cur.x - var->last.x) +
			       sqr (cur.y - var->last.y);

			if (dist > var->delta) {
				/* Do we suspect the previous sample was a noise? */
				if (var->flags & VAR_NOISEVALID) {
					/* Two "noises": it's just a quick pen movement */
					samp [count++] = var->last = var->noise;
					var->flags = (var->flags & ~VAR_NOISEVALID) |
						VAR_SUBMITNOISE;
				} else
					var->flags |= VAR_NOISEVALID;

				/* The pen jumped too far, maybe it's a noise ... */
				var->noise = cur;
				continue;
			} else
				var->flags &= ~VAR_NOISEVALID;
		}

acceptsample:
#ifdef DEBUG
		fprintf(stderr,"VARIANCE----------------> %d %d %d\n",
			var->last.x, var->last.y, var->last.pressure);
#endif
		samp [count++] = var->last;
		var->last = cur;
	}

	return count;
}

static int variance_fini(struct tslib_module_info *info)
{
	free(info);
        return 0;
}

static const struct tslib_ops variance_ops =
{
	.read	= variance_read,
	.fini	= variance_fini,
};

static int variance_limit(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_variance *var = (struct tslib_variance *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul(str, NULL, 0);

	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)data) {
	case 1:
		var->delta = v;
		break;

	default:
		return -1;
	}
	return 0;
}

static const struct tslib_vars variance_vars[] =
{
	{ "delta",	(void *)1, variance_limit },
};

#define NR_VARS (sizeof(variance_vars) / sizeof(variance_vars[0]))

TSAPI struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_variance *var;

	var = malloc(sizeof(struct tslib_variance));
	if (var == NULL)
		return NULL;

	var->module.ops = &variance_ops;

	var->delta = 30;
	var->flags = 0;

	if (tslib_parse_vars(&var->module, variance_vars, NR_VARS, params)) {
		free(var);
		return NULL;
	}

        var->delta = sqr (var->delta);

	return &var->module;
}
