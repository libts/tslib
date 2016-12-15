/*
 *  tslib/plugins/skip.c
 *
 * (C) 2008 by Openmoko, Inc.
 * Author: Nelson Castillo
 * Author: Martin Kepplinger (read_mt())
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
	struct ts_sample_mt **buf_mt;
	int slots;
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

static int skip_read_mt(struct tslib_module_info *info, struct ts_sample_mt **samp, int max_slots, int nr)
{
	struct tslib_skip *skip = (struct tslib_skip *)info;
	int nread = 0;
	int i, j;
	struct ts_sample_mt **cur;
	short pen_up = 0;

	cur = malloc(sizeof(struct ts_sample_mt *));
	if (!cur)
		return -ENOMEM;

	cur[0] = calloc(max_slots, sizeof(struct ts_sample_mt));
	if (!cur[0]) {
		if (cur)
			free(cur);

		return -ENOMEM;
	}

	if (skip->ntail) {
		if (skip->slots < max_slots || skip->buf_mt == NULL) {
			skip->buf_mt = malloc(skip->ntail * sizeof(struct ts_sample_mt *));
			if (!skip->buf_mt) {
				free(cur[0]);
				free(cur);
				return -ENOMEM;
			}

			for (i = 0; i < skip->ntail; i++) {
				skip->buf_mt[i] = calloc(max_slots, sizeof(struct ts_sample_mt));
				if (!skip->buf_mt[i]){
					for (j = 0; j < i; j++)
						free(skip->buf_mt[j]);
					free(skip->buf_mt);
					free(cur[0]);
					free(cur);
					return -ENOMEM;
				}
			}

			skip->slots = max_slots;
		}
	}

	while (nread < nr) {
		if (info->next->ops->read_mt(info->next, cur, max_slots, 1) < 1)
			return nread;

		/* skip the first N samples */
		if (skip->N < skip->nhead) {
			skip->N++;
			for (i = 0; i < max_slots; i++) {
				if (cur[0][i].valid == 1 && cur[0][i].pen_down == 0)
					reset_skip(skip);
			}
			continue;
		}

		/* We didn't send DOWN -- Ignore UP */
		for (i = 0; i < max_slots; i++) {
			if (cur[0][i].valid == 1 && cur[0][i].pen_down == 0 && skip->sent == 0) {
			      reset_skip(skip);
			      continue;
			}
		}

		/* Just accept the sample if ntail is zero */
		if (skip->ntail == 0) {
			memcpy(samp[nread], cur[0], max_slots * sizeof(struct ts_sample_mt));
			nread++;
			skip->sent = 1;
			for (i = 0; i < max_slots; i++) {
				if (cur[0][i].valid == 1 && cur[0][i].pen_down == 0)
					reset_skip(skip);
			}
			continue;
		}

		/* ntail > 0,  Queue current point if we need to */
		if (skip->sent == 0 && skip->M < skip->ntail) {
			memcpy(skip->buf_mt[skip->M], cur[0], max_slots * sizeof(struct ts_sample_mt));
			skip->M++;
			continue;
		}

		/* queue full, accept one, queue one */

		if (skip->M >= skip->ntail) {
			skip->M = 0;
		}

		memcpy(samp[nread], skip->buf_mt[skip->M], max_slots * sizeof(struct ts_sample_mt));
		nread++;

#ifdef DEBUG
		for (i = 0; i < max_slots; i++) {
			if (skip->buf_mt[skip->M][i].valid == 1) {
				fprintf(stderr, "skip---> (Slot %d: X:%d Y:%d) btn_touch:%d\n",
					skip->buf_mt[skip->M][i].slot,
					skip->buf_mt[skip->M][i].x, skip->buf_mt[skip->M][i].y,
					skip->buf_mt[skip->M][i].pen_down);
			}
		}
#endif


		for (i = 0; i < max_slots; i++) {
			if (cur[0][i].valid == 1 && cur[0][i].pen_down == 0) {
				reset_skip(skip);
				pen_up = 1;
			}
		}
		if (pen_up == 1) {
			memcpy(skip->buf_mt[skip->M], cur[0], max_slots * sizeof(struct ts_sample_mt));
			skip->M++;
			skip->sent = 1;
		}
	}

	for (i = 0; i < skip->ntail; i++) {
		free(skip->buf_mt[i]);
	}

	free(cur[0]);
	free(cur);

	return nread;
}

static int skip_fini(struct tslib_module_info *info)
{
	struct tslib_skip *skip = (struct tslib_skip *)info;
	int i;

	if (skip->buf)
		free(skip->buf);

	for (i = 0; i < skip->ntail; i++) {
		if (skip->buf_mt[i])
			free(skip->buf_mt[i]);
	}

	if (skip->buf_mt)
		free(skip->buf_mt);

	free(info);

        return 0;
}

static const struct tslib_ops skip_ops =
{
	.read		= skip_read,
	.read_mt	= skip_read_mt,
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
	skip->buf_mt = NULL;
	skip->slots = 0;

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
