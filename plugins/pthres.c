/*
 *  tslib/plugins/pthres.c
 *
 *  Copyright (C) 2016 Martin Kepplinger <martin.kepplinger@ginzinger.com>
 *  Copyright (C) 2003 Texas Instruments, Inc.
 *
 *  Based on:
 *    tslib/plugins/linear.c
 *    Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Ensure a release is always pressure 0, and that a press is always >=1.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include "config.h"
#include "tslib.h"
#include "tslib-filter.h"

struct tslib_pthres {
	struct tslib_module_info module;
	unsigned int	pmin;
	unsigned int	pmax;
	int		*xsave;
	int		*ysave;
	int		*press;
	int		current_max_slots;
};

static int pthres_read(struct tslib_module_info *info, struct ts_sample *samp,
		       int nr_samples)
{
	struct tslib_pthres *p = (struct tslib_pthres *)info;
	int ret;
	static int xsave, ysave;
	static int press;

	ret = info->next->ops->read(info->next, samp, nr_samples);
	if (ret >= 0) {
		int nr = 0, i;
		struct ts_sample *s;

		for (s = samp, i = 0; i < ret; i++, s++) {
			if (s->pressure < p->pmin) {
				if (press != 0) {
					/* release */
					press = 0;
					s->pressure = 0;
					s->x = xsave;
					s->y = ysave;
				} else {
					/* release with no press,
					 * outside bounds, dropping
					 */
					int left = ret - nr - 1;

					if (left > 0) {
						memmove(s,
							s + 1,
							left * sizeof(struct ts_sample));
						s--;
						continue;
					}
					break;
				}
			} else {
				if (s->pressure > p->pmax) {
					/* pressure outside bounds, dropping */
					int left = ret - nr - 1;

					if (left > 0) {
						memmove(s,
							s + 1,
							left * sizeof(struct ts_sample));
						s--;
						continue;
					}
					break;
				}
				/* press */
				press = 1;
				xsave = s->x;
				ysave = s->y;
			}
			nr++;
		}
		return nr;
	}
	return ret;
}

static int pthres_read_mt(struct tslib_module_info *info,
			  struct ts_sample_mt **samp, int max_slots,
			  int nr_samples)
{
	struct tslib_pthres *p = (struct tslib_pthres *)info;
	int ret;
	int i, j;

	if (p->xsave == NULL || max_slots > p->current_max_slots) {
		free(p->xsave);

		p->xsave = calloc(max_slots, sizeof(int));
		if (!p->xsave)
			return -ENOMEM;

		p->current_max_slots = max_slots;
	}

	if (p->ysave == NULL || max_slots > p->current_max_slots) {
		free(p->ysave);

		p->ysave = calloc(max_slots, sizeof(int));
		if (!p->ysave)
			return -ENOMEM;

		p->current_max_slots = max_slots;
	}

	if (p->press == NULL || max_slots > p->current_max_slots) {
		free(p->press);

		p->press = calloc(max_slots, sizeof(int));
		if (!p->press)
			return -ENOMEM;

		p->current_max_slots = max_slots;
	}

	if (!info->next->ops->read_mt)
		return -ENOSYS;

	ret = info->next->ops->read_mt(info->next, samp, max_slots, nr_samples);
	if (ret < 0)
		return ret;

#ifdef DEBUG
	printf("PTHRES:   read %d samples (mem: %d nr x %d slots)\n",
	       ret, nr_samples, max_slots);
#endif

	for (i = 0; i < ret; i++) {
		for (j = 0; j < max_slots; j++) {
			if (!(samp[i][j].valid & TSLIB_MT_VALID))
				continue;

			if (samp[i][j].pressure < p->pmin) {
				if (p->press[j] != 0) {
					/* release */
					p->press[j] = 0;
					samp[i][j].pressure = 0;
					samp[i][j].x = p->xsave[j];
					samp[i][j].y = p->ysave[j];
				} else {
					/* release with no press,
					 * outside bounds, dropping
					 */
					samp[i][j].valid = 0;
				}
			} else if (samp[i][j].pressure > p->pmax) {
				/* pressure outside bounds, dropping */
				samp[i][j].valid = 0;
			} else {
				/* press */
				p->press[j] = 1;
				p->xsave[j] = samp[i][j].x;
				p->ysave[j] = samp[i][j].y;
			}
		}
	}

	return ret;
}

static int pthres_fini(struct tslib_module_info *info)
{
	struct tslib_pthres *p = (struct tslib_pthres *)info;

	free(p->xsave);
	free(p->ysave);
	free(p->press);

	free(info);

	return 0;
}

static const struct tslib_ops pthres_ops = {
	.read		= pthres_read,
	.read_mt	= pthres_read_mt,
	.fini		= pthres_fini,
};

static int threshold_vars(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_pthres *p = (struct tslib_pthres *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul(str, NULL, 0);

	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)(intptr_t)data) {
	case 0:
		p->pmin = v;
		break;

	case 1:
		p->pmax = v;
		break;

	default:
		return -1;
	}
	return 0;
}


static const struct tslib_vars pthres_vars[] = {
	{ "pmin",	(void *)0, threshold_vars },
	{ "pmax",	(void *)1, threshold_vars }
};

#define NR_VARS (sizeof(pthres_vars) / sizeof(pthres_vars[0]))

TSAPI struct tslib_module_info *pthres_mod_init(__attribute__ ((unused)) struct tsdev *dev,
						const char *params)
{

	struct tslib_pthres *p;

	p = malloc(sizeof(struct tslib_pthres));
	if (p == NULL)
		return NULL;

	p->module.ops = &pthres_ops;

	p->pmin = 1;
	p->pmax = INT_MAX;
	p->xsave = NULL;
	p->ysave = NULL;
	p->press = NULL;
	p->current_max_slots = 0;

	/*
	 * Parse the parameters.
	 */
	if (tslib_parse_vars(&p->module, pthres_vars, NR_VARS, params)) {
		free(p);
		return NULL;
	}

	return &p->module;
}

#ifndef TSLIB_STATIC_PTHRES_MODULE
	TSLIB_MODULE_INIT(pthres_mod_init);
#endif
