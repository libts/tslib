/*
 *  tslib/plugins/skip.c
 *
 * (C) 2017 Martin Kepplinger <martin.kepplinger@ginzinger.com> (read_mt())
 * (C) 2008 by Openmoko, Inc.
 * Author: Nelson Castillo
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
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
	int *N_mt;

	int ntail;
	int M;
	int *M_mt;
	struct ts_sample *buf;
	struct ts_sample_mt **buf_mt;
	struct ts_sample_mt **cur_mt;
	int slots;
	int sent;
	int *sent_mt;
};

static void reset_skip(struct tslib_skip *s)
{
	s->N = 0;
	s->M = 0;
	s->sent = 0;
}

static void reset_skip_mt(struct tslib_skip *s, int slot)
{
	s->N_mt[slot] = 0;
	s->M_mt[slot] = 0;
	s->sent_mt[slot] = 0;
}

static int skip_read(struct tslib_module_info *info, struct ts_sample *samp,
		     int nr)
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
		fprintf(stderr, "SKIP---> (X:%d Y:%d) pressure:%d\n",
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

static int skip_read_mt(struct tslib_module_info *info,
			struct ts_sample_mt **samp, int max_slots, int nr)
{
	struct tslib_skip *skip = (struct tslib_skip *)info;
	int nread = 0;
	int i, j;
	int ret = 0;
	int count = 0;
	int count_current = 0;

	if (skip->cur_mt == NULL || max_slots > skip->slots) {
		if (skip->cur_mt) {
			free(skip->cur_mt[0]);
			free(skip->cur_mt);
		}

		skip->cur_mt = malloc(sizeof(struct ts_sample_mt *));
		if (!skip->cur_mt)
			return -ENOMEM;

		skip->cur_mt[0] = calloc(max_slots,
					 sizeof(struct ts_sample_mt));
		if (!skip->cur_mt[0]) {
			free(skip->cur_mt);
			skip->cur_mt = NULL;

			return -ENOMEM;
		}

		skip->slots = max_slots;
	}
	memset(skip->cur_mt[0], 0, max_slots * sizeof(struct ts_sample_mt));

	if (skip->slots < max_slots || skip->buf_mt == NULL) {
		if (skip->buf_mt) {
			for (i = 0; i < skip->ntail; i++)
				free(skip->buf_mt[i]);

			free(skip->buf_mt);
		}

		skip->buf_mt = malloc(skip->ntail *
				      sizeof(struct ts_sample_mt *));
		if (!skip->buf_mt) {
			free(skip->cur_mt[0]);
			free(skip->cur_mt);
			return -ENOMEM;
		}

		for (i = 0; i < skip->ntail; i++) {
			skip->buf_mt[i] = calloc(max_slots,
						 sizeof(struct ts_sample_mt));
			if (!skip->buf_mt[i]) {
				for (j = 0; j < i; j++)
					free(skip->buf_mt[j]);
				free(skip->buf_mt);
				skip->buf_mt = NULL;
				free(skip->cur_mt[0]);
				free(skip->cur_mt);
				skip->cur_mt = NULL;
				return -ENOMEM;
			}
		}

		skip->N_mt = calloc(max_slots, sizeof(int));
		if (!skip->N_mt) {
			for (i = 0; i < skip->ntail; i++)
				free(skip->buf_mt[i]);
			free(skip->buf_mt);
			skip->buf_mt = NULL;
			free(skip->cur_mt[0]);
			free(skip->cur_mt);
			skip->cur_mt = NULL;
			return -ENOMEM;
		}

		skip->M_mt = calloc(max_slots, sizeof(int));
		if (!skip->M_mt) {
			free(skip->N_mt);
			skip->N_mt = NULL;
			for (i = 0; i < skip->ntail; i++)
				free(skip->buf_mt[i]);
			free(skip->buf_mt);
			skip->buf_mt = NULL;
			free(skip->cur_mt[0]);
			free(skip->cur_mt);
			skip->cur_mt = NULL;
			return -ENOMEM;
		}

		skip->sent_mt = calloc(max_slots, sizeof(int));
		if (!skip->sent_mt) {
			free(skip->N_mt);
			skip->N_mt = NULL;
			free(skip->M_mt);
			skip->M_mt = NULL;
			for (i = 0; i < skip->ntail; i++)
				free(skip->buf_mt[i]);
			free(skip->buf_mt);
			skip->buf_mt = NULL;
			free(skip->cur_mt[0]);
			free(skip->cur_mt);
			skip->cur_mt = NULL;
			return -ENOMEM;
		}

		skip->slots = max_slots;
	}

	if (!info->next->ops->read_mt)
		return -ENOSYS;

	ret = info->next->ops->read_mt(info->next, samp, max_slots, nr);
	if (ret < 0)
		return ret;

#ifdef DEBUG
	printf("SKIP: read %d samples (%d slots)\n",
	       ret, max_slots);
#endif
	while (nread < ret) {
		count_current = 0;
		memcpy(skip->cur_mt[0], samp[count],
		       max_slots * sizeof(struct ts_sample_mt));
		for (i = 0; i < max_slots; i++) {
			if (!(skip->cur_mt[0][i].valid & TSLIB_MT_VALID))
				continue;

			/* skip the first N samples */
			if (skip->N_mt[i] < skip->nhead) {
			#ifdef DEBUG
				printf("SKIP: (slot %d) skip %d of %d samples\n",
				       i, skip->N_mt[i] + 1, skip->nhead);
			#endif
				skip->N_mt[i]++;
				if (skip->cur_mt[0][i].pressure == 0) {
					reset_skip_mt(skip, i);
				}

				skip->cur_mt[0][i].valid = 0;
				samp[count][i].valid = 0;
				continue;
			}

			/* We didn't send DOWN -- Ignore UP */
			if (skip->cur_mt[0][i].pressure == 0 &&
			    skip->sent_mt[i] == 0) {
			#ifdef DEBUG
				fprintf(stderr, "SKIP: (Slot %d) ignore up\n", i);
			#endif
				reset_skip_mt(skip, i);
				continue;
			}

			/* Just accept the sample if ntail is zero */
			if (skip->ntail == 0) {

				if (skip->sent_mt[i] == 0) {
					skip->cur_mt[0][i].pen_down = 1;
					skip->cur_mt[0][i].valid |= TSLIB_MT_VALID;
				}

				memcpy(&samp[count][i], &skip->cur_mt[0][i],
				       sizeof(struct ts_sample_mt));

				if (count_current == 0) {
					nread++;
				}
				count_current++;

				skip->sent_mt[i] = 1;
				if (skip->cur_mt[0][i].pressure == 0)
					reset_skip_mt(skip, i);
			#ifdef DEBUG
				fprintf(stderr,
					"SKIP: ntail 0 - sample accepted\n");
			#endif
				continue;
			}

			/* ntail > 0,  Queue current point if we need to */
			if (skip->sent_mt[i] == 0 && skip->M_mt[i] < skip->ntail) {
				skip->cur_mt[0][i].pen_down = 1;
				skip->cur_mt[0][i].valid |= TSLIB_MT_VALID;
				samp[count][i].valid = 0;

			#ifdef DEBUG
				fprintf(stderr,
					"SKIP: queue one sample to %d\n",
					skip->M_mt[i]);
			#endif
				memcpy(&skip->buf_mt[skip->M_mt[i]][i],
				       &skip->cur_mt[0][i],
				       sizeof(struct ts_sample_mt));
				skip->M_mt[i]++;
				continue;
			}
			/* queue full, accept one, queue one */
			if (skip->M_mt[i] >= skip->ntail)
				skip->M_mt[i] = 0;


			if (skip->cur_mt[0][i].pressure == 0) {
				skip->buf_mt[skip->M_mt[i]][i].pressure = 0;
				skip->buf_mt[skip->M_mt[i]][i].pen_down = 0;
				skip->buf_mt[skip->M_mt[i]][i].tracking_id = -1;
				skip->buf_mt[skip->M_mt[i]][i].valid |= TSLIB_MT_VALID;
			}

			memcpy(&samp[count][i], &skip->buf_mt[skip->M_mt[i]][i],
			       sizeof(struct ts_sample_mt));

	#ifdef DEBUG
			fprintf(stderr,
				"SKIP: (Slot %d) X:%4d Y:%4d pressure:%d btn_touch:%d\n",
				skip->buf_mt[skip->M_mt[i]][i].slot,
				skip->buf_mt[skip->M_mt[i]][i].x,
				skip->buf_mt[skip->M_mt[i]][i].y,
				skip->buf_mt[skip->M_mt[i]][i].pressure,
				skip->buf_mt[skip->M_mt[i]][i].pen_down);

	#endif
			if (count_current == 0) {
				nread++;
			}
			count_current++;

			if (skip->cur_mt[0][i].pressure == 0) {
				reset_skip_mt(skip, i);
			} else {
				memcpy(&skip->buf_mt[skip->M_mt[i]][i],
				       &skip->cur_mt[0][i],
				       sizeof(struct ts_sample_mt));


			#ifdef DEBUG
				fprintf(stderr,
					"SKIP: accept and queue one sample (slot %d) to %d\n", i, skip->M_mt[i]);
			#endif
				skip->sent_mt[i] = 1;
				skip->M_mt[i]++;
			}

		}
		count++;
		if (count == ret)
			break;


	}

	return nread;
}

static int skip_fini(struct tslib_module_info *info)
{
	struct tslib_skip *skip = (struct tslib_skip *)info;
	int i;

	free(skip->N_mt);
	free(skip->M_mt);
	free(skip->sent_mt);
	free(skip->buf);

	if (skip->buf_mt) {
		for (i = 0; i < skip->ntail; i++)
			free(skip->buf_mt[i]);

		free(skip->buf_mt);
	}

	if (skip->cur_mt) {
		free(skip->cur_mt[0]);
		free(skip->cur_mt);
	}

	free(info);

	return 0;
}

static const struct tslib_ops skip_ops = {
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

static const struct tslib_vars skip_vars[] = {
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
	skip->cur_mt = NULL;
	skip->slots = 0;
	skip->N_mt = NULL;
	skip->M_mt = NULL;
	skip->sent_mt = NULL;

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
