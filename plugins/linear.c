/*
 *  tslib/plugins/linear.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: linear.c,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Linearly scale touchscreen values
 */
#include <stdlib.h>
#include <string.h>

#include "tslib.h"
#include "tslib-filter.h"

struct tslib_linear {
	struct tslib_module_info module;
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

struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_linear *lin;
	char *tok, *str, *p;

	if (params) {
		str = p = strdup(params);
		if (!p)
			return NULL;
	} else
		str = p = NULL;

	lin = malloc(sizeof(struct tslib_linear));
	if (lin == NULL) {
		if (str)
			free(str);
		return NULL;
	}

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
	while ((tok = strsep(&p, " \t")) != NULL) {

	}

	if (str)
		free(str);

	return &lin->module;
}
