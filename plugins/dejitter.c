/*
 *  tslib/plugins/threshold.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: dejitter.c,v 1.6 2002/11/08 23:28:55 dlowder Exp $
 *
 * Threshold filter for touchscreen values
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <stdio.h>

#include "tslib.h"
#include "tslib-filter.h"

#define NR_LAST	4

struct tslib_threshold {
	struct tslib_module_info	module;
	int			pthreshold;
	int			xdelta;
	int			ydelta;
	int			delta2;
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
			int dr2;
#ifdef DEBUG
			fprintf(stderr,"BEFORE DEJITTER---------------> %d %d %d\n",s->x,s->y,s->pressure);
#endif /*DEBUG*/
			thr->down = (s->pressure >= thr->pthreshold);
			if (thr->down) {
				dr2 = (thr->x - s->x)*(thr->x - s->x) 
					+ (thr->y - s->y)*(thr->y - s->y);
				if(dr2 < thr->delta2) {
					s->x = thr->x;
					s->y = thr->y;
				} else {
					thr->x = s->x;
					thr->y = s->y;
				}

			} else {
				s->x = thr->x;
				s->y = thr->y;
			}


			samp[nr++] = *s;
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

//#define NR_VARS (sizeof(threshold_vars) / sizeof(threshold_vars[0]))
#define NR_VARS 3

struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_threshold *thr;

	thr = malloc(sizeof(struct tslib_threshold));
	if (thr == NULL)
		return NULL;

	thr->module.ops = &threshold_ops;

	thr->xdelta = 10;
	thr->ydelta = 10;
	thr->pthreshold = 100;

	if (tslib_parse_vars(&thr->module, threshold_vars, NR_VARS, params)) {
		free(thr);
		return NULL;
	}
	thr->delta2 = (thr->xdelta)*(thr->xdelta) + (thr->ydelta)*(thr->ydelta);

	return &thr->module;
}
