/*
 *  tslib/plugins/pthres.c
 *
 *  Copyright (C) 2003 Texas Instruments, Inc.
 *
 *  Based on:
 *    tslib/plugins/linear.c
 *    Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * Ensure a release is always pressure 0, and that a press is always >=1.
 */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "tslib.h"
#include "tslib-filter.h"

struct tslib_pthres {
	struct tslib_module_info module;
	unsigned int	pmin;
	unsigned int	pmax;
};

static int
pthres_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_pthres *p = (struct tslib_pthres *)info;
	int ret;
	static int xsave = 0, ysave = 0;
	static int press = 0;

	ret = info->next->ops->read(info->next, samp, nr);
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
					/* release with no press, outside bounds, dropping */
					int left = ret - nr - 1;
					if (left > 0) {
						memmove(s, s + 1, left * sizeof(struct ts_sample));
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
						memmove(s, s + 1, left * sizeof(struct ts_sample));
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

static int pthres_fini(struct tslib_module_info *info)
{
	free(info);
	return 0;
}

static const struct tslib_ops pthres_ops =
{
	.read	= pthres_read,
	.fini	= pthres_fini,
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
	switch ((int)data) {
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


static const struct tslib_vars pthres_vars[] =
{
	{ "pmin",	(void *)0, threshold_vars },
	{ "pmax",	(void *)1, threshold_vars }
};

#define NR_VARS (sizeof(pthres_vars) / sizeof(pthres_vars[0]))

TSAPI struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{

	struct tslib_pthres *p;

	p = malloc(sizeof(struct tslib_pthres));
	if (p == NULL)
		return NULL;

	p->module.ops = &pthres_ops;

	p->pmin = 1;
	p->pmax = INT_MAX;

	/*
	 * Parse the parameters.
	 */
	if (tslib_parse_vars(&p->module, pthres_vars, NR_VARS, params)) {
		free(p);
		return NULL;
	}

	return &p->module;
}
