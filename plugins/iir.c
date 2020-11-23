/*
 *  tslib/plugins/iir.c
 *
 *  Copyright (C) 2017 Martin Kepplinger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Similar to dejitter, this is a smoothing filter to remove low-level noise.
 * See https://en.wikipedia.org/wiki/Infinite_impulse_response for some theory.
 * There is a trade-off between noise removal (smoothing) and responsiveness:
 * The parameters N and D specify the level of smoothing in the form of a
 * fraction (N/D).
 *
 * The discrete formula we use is found at http://dlbeer.co.nz/articles/tsf.html
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

struct tslib_iir {
	struct tslib_module_info module;
	uint32_t D;
	uint32_t N;
	int32_t slots;
	uint32_t nr;
	int32_t s;
	int32_t t;
	uint8_t last_active; /* for finding pen-down */

	int32_t *s_mt;
	int32_t *t_mt;
	uint8_t *last_active_mt; /* for finding pen-down */
};

static void iir_filter(struct tslib_module_info *info, int32_t *new,
		       int32_t *save)
{
	struct tslib_iir *iir = (struct tslib_iir *)info;

	*save = (iir->N * *save + (iir->D - iir->N) * *new + iir->D / 2) / iir->D;
}

/* legacy read interface */
static int iir_read(struct tslib_module_info *info, struct ts_sample *samp,
		    int nr)
{
	struct tslib_iir *iir = (struct tslib_iir *)info;
	int32_t ret;
	int32_t i;

	ret = info->next->ops->read(info->next, samp, nr);
	if (ret < 0)
		return ret;

	for (i = 0; i < ret; i++, samp++) {
		if (samp->pressure == 0) { /* reset */
			iir->s = samp->x;
			iir->t = samp->y;
			iir->last_active = 0;
			continue;
		} else if (iir->last_active == 0) {
			iir->s = samp->x;
			iir->t = samp->y;
			iir->last_active = 1;
			continue;
		}

		iir_filter(info, &samp->x, &iir->s);
	#ifdef DEBUG
		printf("IIR: X: %d -> %d\n", samp->x, iir->s);
	#endif
		samp->x = iir->s;

		iir_filter(info, &samp->y, &iir->t);
	#ifdef DEBUG
		printf("IIR: Y: %d -> %d\n", samp->y, iir->t);
	#endif
		samp->y = iir->t;
	}

	return ret;
}

static int iir_read_mt(struct tslib_module_info *info,
		       struct ts_sample_mt **samp, int max_slots, int nr)
{
	struct tslib_iir *iir = (struct tslib_iir *)info;
	int32_t ret;
	int32_t i, j;

	if (!iir->s_mt || max_slots > iir->slots) {
		free(iir->s_mt);
		free(iir->t_mt);
		free(iir->last_active_mt);

		iir->s_mt = calloc(max_slots, sizeof(int32_t));
		if (!iir->s_mt)
			return -ENOMEM;

		iir->t_mt = calloc(max_slots, sizeof(int32_t));
		if (!iir->t_mt)
			return -ENOMEM;

		iir->last_active_mt = calloc(max_slots, sizeof(uint8_t));
		if (!iir->last_active_mt)
			return -ENOMEM;

		iir->slots = max_slots;
	}

	if (!info->next->ops->read_mt)
		return -ENOSYS;

	ret = info->next->ops->read_mt(info->next, samp, max_slots, nr);
	if (ret < 0)
		return ret;

	for (i = 0; i < ret; i++) {
		for (j = 0; j < max_slots; j++) {
			if (!(samp[i][j].valid & TSLIB_MT_VALID))
				continue;
			/* reset */
			if (samp[i][j].pressure == 0) { /* pen-up */
				iir->s_mt[j] = samp[i][j].x;
				iir->t_mt[j] = samp[i][j].y;
				iir->last_active_mt[j] = 0;
				continue;
			} else if (iir->last_active_mt[j] == 0) { /* pen-down */
				iir->s_mt[j] = samp[i][j].x;
				iir->t_mt[j] = samp[i][j].y;
				iir->last_active_mt[j] = 1;
				continue;
			}

			iir_filter(info, &samp[i][j].x, &iir->s_mt[j]);
		#ifdef DEBUG
			printf("IIR: (slot %d) X: %d -> %d\n",
			       j, samp[i][j].x, iir->s_mt[j]);
		#endif
			samp[i][j].x = iir->s_mt[j];

			iir_filter(info, &samp[i][j].y, &iir->t_mt[j]);
		#ifdef DEBUG
			printf("IIR: (slot %d) Y: %d -> %d\n",
			       j, samp[i][j].y, iir->t_mt[j]);
		#endif
			samp[i][j].y = iir->t_mt[j];
		}
	}

	return ret;
}

static int iir_fini(struct tslib_module_info *info)
{
	struct tslib_iir *iir = (struct tslib_iir *)info;

	free(iir->s_mt);
	free(iir->t_mt);
	free(iir->last_active_mt);

	free(info);

	return 0;
}

static const struct tslib_ops iir_ops = {
	.read		= iir_read,
	.read_mt	= iir_read_mt,
	.fini		= iir_fini,
};

static int iir_opt(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_iir *iir = (struct tslib_iir *)inf;
	unsigned long v;
	int32_t err = errno;

	v = strtoul(str, NULL, 0);

	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)(intptr_t)data) {
	case 1:
		iir->N = v;
		break;

	case 2:
		iir->D = v;
		if (iir->D == 0) {
			printf("IIR: avoid division by zero: D=1 set\n");
			iir->D = 1;
		}
		break;

	default:
		return -1;
	}


	return 0;
}

static const struct tslib_vars iir_vars[] = {
	{ "N",	(void *)1, iir_opt },
	{ "D",	(void *)2, iir_opt },
};

#define NR_VARS (sizeof(iir_vars) / sizeof(iir_vars[0]))

TSAPI struct tslib_module_info *iir_mod_init(__attribute__ ((unused)) struct tsdev *dev,
					     const char *params)
{
	struct tslib_iir *iir;

	iir = malloc(sizeof(struct tslib_iir));
	if (iir == NULL)
		return NULL;

	memset(iir, 0, sizeof(struct tslib_iir));
	iir->module.ops = &iir_ops;

	iir->N = 0;
	iir->D = 0;
	iir->slots = 0;
	iir->nr = 0;
	iir->s = 0;
	iir->t = 0;
	iir->s_mt = NULL;
	iir->t_mt = NULL;
	iir->last_active = 0;

	if (tslib_parse_vars(&iir->module, iir_vars, NR_VARS, params)) {
		free(iir);
		return NULL;
	}

	return &iir->module;
}

#ifndef TSLIB_STATIC_IIR_MODULE
	TSLIB_MODULE_INIT(iir_mod_init);
#endif
