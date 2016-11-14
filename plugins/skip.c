/*
 *  tslib/plugins/skip.c
 *
 * (C) 2008 by Openmoko, Inc.
 * Author: Nelson Castillo
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * Skip filter for touchscreen values.
 *
 * Problem: With some drivers the first and the last sample is reliable.
 *
 * Solution:
 *
 *  - Skip N points after pressure != 0
 *  - Skip M points before pressure == 0
 *  - Ignore a click if it has less than N + M + 1 points
 *
 * Parameters:
 *
 *  - nhead (means N)
 *  - ntail (means M)
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "config.h"
#include "tslib.h"
#include "tslib-filter.h"

struct tslib_skip {
	struct tslib_module_info module;

	int nhead;
	int N;

	int ntail;
	int M;
	struct ts_sample *buf;
	int sent;
};

static void reset_skip(struct tslib_skip *s)
{
	s->N = 0;
	s->M = 0;
	s->sent = 0;
}

static int skip_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_skip *skip = (struct tslib_skip *)info;
	int nread = 0;

	while (nread < nr) {
		struct ts_sample cur;

		if (info->next->ops->read(info->next, &cur, 1) < 1)
			return nread;

		/* skip the first N samples */
		if (skip->N < skip->nhead) {
			skip->N++;
			if (cur.pressure == 0)
				reset_skip(skip);
			continue;
		}

		/* We didn't send DOWN -- Ignore UP */
		if (cur.pressure == 0 && skip->sent == 0) {
			reset_skip(skip);
			continue;
		}

		/* Just accept the sample if ntail is zero */
		if (skip->ntail == 0) {
			samp[nread++] = cur;
			skip->sent = 1;
			if (cur.pressure == 0)
				reset_skip(skip);
			continue;
		}

		/* ntail > 0,  Queue current point if we need to */
		if (skip->sent == 0 && skip->M < skip->ntail) {
			skip->buf[skip->M++] = cur;
			continue;
		}

		/* queue full, accept one, queue one */

		if (skip->M >= skip->ntail)
			skip->M = 0;

		if (cur.pressure == 0)
			skip->buf[skip->M].pressure = 0;

		samp[nread++] = skip->buf[skip->M];

#ifdef DEBUG
		fprintf(stderr, "skip---> (X:%d Y:%d) pressure:%d\n",
			skip->buf[skip->M].x, skip->buf[skip->M].y,
			skip->buf[skip->M].pressure);
#endif

		if (cur.pressure == 0) {
			reset_skip(skip);
		} else {
			skip->buf[skip->M++] = cur;
			skip->sent = 1;
		}
	}
	return nread;
}

static int skip_fini(struct tslib_module_info *info)
{
	struct tslib_skip *skip = (struct tslib_skip *)info;

	if (skip->buf)
		free(skip->buf);

	free(info);

        return 0;
}

static const struct tslib_ops skip_ops =
{
	.read		= skip_read,
	.fini		= skip_fini,
};

static int skip_opt(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_skip *skip = (struct tslib_skip *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul(str, NULL, 0);

	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;

	switch ((int)(intptr_t)data) {
	case 1:
		skip->nhead = v;
		break;

	case 2:
		skip->ntail = v;
		break;

	default:
		return -1;
	}
	return 0;
}

static const struct tslib_vars skip_vars[] =
{
	{ "nhead",      (void *)1, skip_opt },
	{ "ntail",      (void *)2, skip_opt },
};

#define NR_VARS (sizeof(skip_vars) / sizeof(skip_vars[0]))

TSAPI struct tslib_module_info *skip_mod_init(__attribute__ ((unused)) struct tsdev *dev,
					      const char *params)
{
	struct tslib_skip *skip;

	skip = malloc(sizeof(struct tslib_skip));
	if (skip == NULL)
		return NULL;

	memset(skip, 0, sizeof(struct tslib_skip));
	skip->module.ops = &skip_ops;

	skip->nhead = 1; /* by default remove the first */
	skip->ntail = 1; /* by default remove the last */
	skip->buf = NULL;

	reset_skip(skip);

	if (tslib_parse_vars(&skip->module, skip_vars, NR_VARS, params)) {
		free(skip);
		return NULL;
	}

	if (skip->ntail &&
	    !(skip->buf = malloc(sizeof(struct ts_sample) * skip->ntail))) {
		free(skip);
		return NULL;
	}

	return &skip->module;
}

#ifndef TSLIB_STATIC_SKIP_MODULE
	TSLIB_MODULE_INIT(skip_mod_init);
#endif
