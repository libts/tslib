/*
 *  tslib/plugins/linear.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: linear.c,v 1.2 2002/01/15 13:56:48 rmk Exp $
 *
 * Linearly scale touchscreen values
 */
#include <stdlib.h>
#include <string.h>

#include "tslib.h"
#include "tslib-filter.h"

struct tslib_linear {
	struct tslib_module_info module;
	int	swap_xy;
	int	x_offset;
	int	x_mult;
	int	x_div;
	int	y_offset;
	int	y_mult;
	int	y_div;
	int	p_offset;
	int	p_mult;
	int	p_div;
};

static int
linear_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_linear *lin = (struct tslib_linear *)info;
	int ret;

	ret = info->next->ops->read(info->next, samp, nr);
	if (ret >= 0) {
		int nr;

		for (nr = 0; nr < ret; nr++, samp++) {
			samp->x = ((samp->x + lin->x_offset) * lin->x_mult)
					/ lin->x_div;
			samp->y = ((samp->y + lin->y_offset) * lin->y_mult)
					/ lin->y_div;
			samp->pressure = ((samp->pressure + lin->p_offset)
						 * lin->p_mult) / lin->p_div;
			if (lin->swap_xy) {
				int tmp = samp->x;
				samp->x = samp->y;
				samp->y = tmp;
			}
		}
	}

	return ret;
}

static int linear_fini(struct tslib_module_info *info)
{
	free(info);
}

static const struct tslib_ops linear_ops =
{
	read:		linear_read,
	fini:		linear_fini,
};

static int linear_xyswap(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_linear *lin = (struct tslib_linear *)inf;

	lin->swap_xy = (int)data;
	return 0;
}

static const struct tslib_vars linear_vars[] =
{
	{ "noxyswap",	(void *)0, linear_xyswap },
	{ "xyswap",	(void *)1, linear_xyswap }
};

#define NR_VARS (sizeof(linear_vars) / sizeof(linear_vars[0]))

struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_linear *lin;

	lin = malloc(sizeof(struct tslib_linear));
	if (lin == NULL)
		return NULL;

	lin->module.ops = &linear_ops;

	lin->x_offset = -908;
	lin->x_mult   = 270;
	lin->x_div    = -684;

	lin->y_offset = -918;
	lin->y_mult   = 190;
	lin->y_div    = -664;

	lin->p_offset = 0;
	lin->p_mult   = 0x10000;
	lin->p_div    = 0x10000;

	/*
	 * Parse the parameters.
	 */
	if (tslib_parse_vars(&lin->module, linear_vars, NR_VARS, params)) {
		free(lin);
		return NULL;
	}

	return &lin->module;
}
