/*
 * tslib/plugins/invert.c
 *
 * Copyright 2017 Martin Kepplinger <martink@posteo.de>
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Simply inverts values in X and/or Y direction around given point
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

struct tslib_invert {
	struct		tslib_module_info module;
	int32_t		x0;
	int32_t		y0;
	uint8_t		invert_x;
	uint8_t		invert_y;
};

static int invert_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_invert *ctx = (struct tslib_invert *)info;
	int count = 0;

	while (count < nr) {
		if (info->next->ops->read(info->next, samp, 1) < 1)
			return count;

		if (ctx->invert_x)
			samp->x = ctx->x0 - samp->x;

		if (ctx->invert_y)
			samp->y = ctx->y0 - samp->y;

		count++;
	}
	return count;
}

static int invert_read_mt(struct tslib_module_info *info,
			  struct ts_sample_mt **samp,
			  int max_slots, int nr)
{
	struct tslib_invert *ctx = (struct tslib_invert *)info;
	int ret;
	int i, j;

	if (!info->next->ops->read_mt)
		return -ENOSYS;

	ret = info->next->ops->read_mt(info->next, samp, max_slots, nr);
	if (ret < 0)
		return ret;

#ifdef DEBUG
	if (ret == 0)
		fprintf(stderr, "INVERT: couldn't read data\n");

	printf("INVERT: read %d samples (mem: %d nr x %d slots)\n",
		ret, nr, max_slots);
#endif

	for (i = 0; i < ret; i++) {
		for (j = 0; j < max_slots; j++) {
			if (!(samp[i][j].valid & TSLIB_MT_VALID))
				continue;

			if (ctx->invert_x)
				samp[i][j].x = ctx->x0 - samp[i][j].x;

			if (ctx->invert_y)
				samp[i][j].y = ctx->y0 - samp[i][j].y;

		#ifdef DEBUG
			printf("INVERT: new val slot %d:  X %d  Y %d\n",
				j, samp[i][j].x, samp[i][j].y);
		#endif
		}
	}

	return ret;
}

static int invert_fini(struct tslib_module_info *info)
{
	free(info);

	return 0;
}

static const struct tslib_ops invert_ops = {
	.read		= invert_read,
	.read_mt	= invert_read_mt,
	.fini		= invert_fini,
};

static int invert_opt(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_invert *ctx = (struct tslib_invert *)inf;
	long v;
	int32_t err = errno;

	v = strtol(str, NULL, 0);

	if (v == LONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)(intptr_t)data) {
	case 1:
		ctx->invert_x = 1;
		ctx->x0 = v;
		break;

	case 2:
		ctx->invert_y = 1;
		ctx->y0 = v;
		break;

	default:
		return -1;
	}

	return 0;
}

static const struct tslib_vars invert_vars[] = {
	{ "x0",	(void *)1, invert_opt },
	{ "y0", (void *)2, invert_opt },
};

#define NR_VARS (sizeof(invert_vars) / sizeof(invert_vars[0]))

TSAPI struct tslib_module_info *invert_mod_init(__attribute__ ((unused)) struct tsdev *dev,
						const char *params)
{
	struct tslib_invert *ctx;

	ctx = malloc(sizeof(struct tslib_invert));
	if (ctx == NULL)
		return NULL;

	memset(ctx, 0, sizeof(*ctx));
	ctx->module.ops = &invert_ops;

	ctx->invert_x = 0;
	ctx->invert_y = 0;

	if (tslib_parse_vars(&ctx->module, invert_vars, NR_VARS, params)) {
		free(ctx);
		return NULL;
	}

	return &ctx->module;
}

#ifndef TSLIB_STATIC_INVERT_MODULE
	TSLIB_MODULE_INIT(invert_mod_init);
#endif
