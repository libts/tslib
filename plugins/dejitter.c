/*
 *  tslib/plugins/threshold.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: dejitter.c,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Threshold filter for touchscreen values
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "tslib.h"
#include "tslib-filter.h"

#define NR_LAST	4

struct tslib_threshold {
	struct tslib_module_info	module;
	unsigned int			pthreshold;
	unsigned int			xdelta;
	unsigned int			ydelta;
	unsigned int			x;
	unsigned int			y;
	unsigned int			down;
};

static int threshold_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_threshold *thr = (struct tslib_threshold *)info;
	struct ts_sample *s;
	int ret;

	ret = info->next->ops->read(info->next, samp, nr);
	if (ret >= 0) {
		int nr = 0;

		for (s = samp; s < samp + ret; s++) {
			int dx, dy;

			if (thr->down) {
				dx = thr->x - s->x;
				if (dx < 0)
					dx = -dx;
				dy = thr->y - s->y;
				if (dy < 0)
					dy = -dy;

				if (dx < thr->xdelta)
					s->x = thr->x;
				if (dy < thr->ydelta)
					s->y = thr->y;

				if (thr->x != s->x || thr->y != s->y)
					samp[nr++] = *s;
			}

			thr->down = s->pressure >= thr->pthreshold;

			thr->x = s->x;
			thr->y = s->y;
		}

		ret = nr;
	}
	return ret;
}

static int threshold_fini(struct tslib_module_info *info)
{
	free(info);
}

static const struct tslib_ops threshold_ops =
{
	read:	threshold_read,
	fini:	threshold_fini,
};

static int threshold_limit(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_threshold *thr = (struct tslib_threshold *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul(str, NULL, 0);

	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)data) {
	case 1:
		thr->xdelta = v;
		break;

	case 2:
		thr->ydelta = v;
		break;

	case 3:
		thr->pthreshold = v;
		break;

	default:
		return -1;
	}
	return 0;
}

static const struct tslib_vars threshold_vars[] =
{
	{ "xdelta",	(void *)1, threshold_limit },
	{ "ydelta",	(void *)2, threshold_limit },
	{ "pthreshold",	(void *)3, threshold_limit }
};

#define NR_VARS (sizeof(threshold_vars) / sizeof(threshold_vars[0]))

struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_threshold *thr;

	thr = malloc(sizeof(struct tslib_threshold));
	if (thr == NULL)
		return NULL;

	thr->module.ops = &threshold_ops;

	thr->xdelta = 3;
	thr->ydelta = 3;
	thr->pthreshold = 100;

	if (tslib_parse_vars(&thr->module, threshold_vars, NR_VARS, params)) {
		free(thr);
		return NULL;
	}

	return &thr->module;
}
