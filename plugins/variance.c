/*
 *  tslib/plugins/variance.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: variance.c,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Variance filter for touchscreen values
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "tslib.h"
#include "tslib-filter.h"

#define NR_LAST	4

struct tslib_variance {
	struct tslib_module_info	module;
	int				nr;
	unsigned int			pthreshold;
	unsigned int			xlimit;
	unsigned int			ylimit;
	struct ts_sample		last[NR_LAST];
};

/*
 * We have 4 complete samples.  Calculate the variance between each,
 * treating X and Y values separately.  Then pick the two with the
 * least variance, and average them.
 */
static int
variance_calculate(struct tslib_variance *var, struct ts_sample *samp,
		   struct ts_sample *s)
{
	int i, j;
	int diff_x, min_x, i_x, j_x;
	int diff_y, min_y, i_y, j_y;
	int diff_p, min_p, i_p, j_p;

	min_x = INT_MAX;
	min_y = INT_MAX;
	min_p = INT_MAX;

	for (i = 0; i < var->nr - 1; i++) {
		for (j = i + 1; j < var->nr; j++) {
			/*
			 * Calculate the variance between sample 'i'
			 * and sample 'j'.  X and Y values are treated
			 * separately.
			 */
			diff_x = var->last[i].x - var->last[j].x;
			if (diff_x < 0)
				diff_x = -diff_x;

			diff_y = var->last[i].y - var->last[j].y;
			if (diff_y < 0)
				diff_y = -diff_y;

			diff_p = var->last[i].pressure - var->last[j].pressure;
			if (diff_p < 0)
				diff_p = -diff_p;

			/*
			 * Is the variance between any two samples too large?
			 */
			if (diff_x > var->xlimit || diff_y > var->ylimit)
				return 0;

			/*
			 * Find the minimum X variance.
			 */
			if (min_x > diff_x) {
				min_x = diff_x;
				i_x = i;
				j_x = j;
			}

			/*
			 * Find the minimum Y variance.
			 */
			if (min_y > diff_y) {
				min_y = diff_y;
				i_y = i;
				j_y = j;
			}

			if (min_p > diff_p) {
				min_p = diff_p;
				i_p = i;
				j_p = j;
			}
		}
	}

	samp->x		 = (var->last[i_x].x + var->last[j_x].x) / 2;
	samp->y		 = (var->last[i_y].y + var->last[j_y].y) / 2;
	samp->pressure   = (var->last[i_p].pressure + var->last[j_p].pressure) / 2;
	samp->tv.tv_sec  = s->tv.tv_sec;
	samp->tv.tv_usec = s->tv.tv_usec;

	return 1;
}

static int variance_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_variance *var = (struct tslib_variance *)info;
	struct ts_sample *s;
	int ret;

	ret = info->next->ops->read(info->next, samp, nr);
	if (ret >= 0) {
		int nr = 0;

		for (s = samp; s < samp + ret; s++) {
			if (s->pressure < var->pthreshold) {
				/*
				 * Pen was released.  Reset our state and
				 * pass up the release information.
				 */
				samp[nr].x = 0;
				samp[nr].y = 0;
				samp[nr].pressure = s->pressure;
				samp[nr].tv.tv_sec = s->tv.tv_sec;
				samp[nr].tv.tv_usec = s->tv.tv_usec;

				nr++;

				var->nr = 0;
				continue;
			} else if (var->nr == -1) {
				/*
				 * Pen was pressed.  Inform upper layers
				 * immediately.
				 */
				samp[nr] = *s;
				nr++;
			}

			if (var->nr >= 0) {
				var->last[var->nr].x = s->x;
				var->last[var->nr].y = s->y;
				var->last[var->nr].pressure = s->pressure;
			}

			var->nr++;

			if (var->nr == NR_LAST) {
				if (variance_calculate(var, samp + nr, s))
					nr++;
				var->nr = 0;
			}
		}

		ret = nr;
	}
	return ret;
}

static int variance_fini(struct tslib_module_info *info)
{
	free(info);
}

static const struct tslib_ops variance_ops =
{
	read:	variance_read,
	fini:	variance_fini,
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
		var->xlimit = v;
		break;

	case 2:
		var->ylimit = v;
		break;

	case 3:
		var->pthreshold = v;
		break;

	default:
		return -1;
	}
	return 0;
}

static const struct tslib_vars variance_vars[] =
{
	{ "xlimit",	(void *)1, variance_limit },
	{ "ylimit",	(void *)2, variance_limit },
	{ "pthreshold",	(void *)3, variance_limit }
};

#define NR_VARS (sizeof(variance_vars) / sizeof(variance_vars[0]))

struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_variance *var;

	var = malloc(sizeof(struct tslib_variance));
	if (var == NULL)
		return NULL;

	var->module.ops = &variance_ops;

	var->nr = -1;
	var->xlimit = 160;
	var->ylimit = 160;
	var->pthreshold = 100;

	if (tslib_parse_vars(&var->module, variance_vars, NR_VARS, params)) {
		free(var);
		return NULL;
	}

	return &var->module;
}
