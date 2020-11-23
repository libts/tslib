/*
 * tslib/plugins/lowpass.c
 *
 * Copyright 2017 Martin Kepplinger <martink@posteo.de>
 * Courtesy of Openmoko, 2011
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Very stupid lowpass dejittering filter
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

struct tslib_lowpass {
	struct tslib_module_info module;
	struct ts_sample last;
	struct ts_sample ideal;
	struct ts_sample_mt *last_mt;
	struct ts_sample_mt *ideal_mt;
	int slots;
	float factor;
	unsigned int flags;
	unsigned int *flags_mt;
	unsigned char threshold;
#define VAR_PENUP		0x00000001
};

static int lowpass_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_lowpass *var = (struct tslib_lowpass *)info;
	struct ts_sample current;
	int count = 0;
	int delta;

	while (count < nr) {
		if (info->next->ops->read(info->next, &current, 1) < 1)
			return count;

		if (current.pressure == 0) {
			var->flags |= VAR_PENUP;
			samp[count++] = current;
		} else if (var->flags & VAR_PENUP) {
			var->flags &= ~VAR_PENUP;
			var->last = current;
			samp[count++] = current;
		} else {
			var->ideal = current;

			var->ideal.x = var->last.x;
			delta = current.x - var->last.x;
			if (delta <= var->threshold &&
					delta >= -var->threshold)
				delta = 0;
			delta *= var->factor;
			var->ideal.x += delta;

			var->ideal.y = var->last.y;
			delta = current.y - var->last.y;
			if (delta <= var->threshold &&
					delta >= -var->threshold)
				delta = 0;
			delta *= var->factor;
			var->ideal.y += delta;

			var->last = var->ideal;
			samp[count++] = var->ideal;
		}
	}
	return count;
}

static int lowpass_read_mt(struct tslib_module_info *info,
			   struct ts_sample_mt **samp,
			   int max_slots, int nr)
{
	struct tslib_lowpass *var = (struct tslib_lowpass *)info;
	int delta;
	int ret;
	int i, j;

	if (!info->next->ops->read_mt)
		return -ENOSYS;

	ret = info->next->ops->read_mt(info->next, samp, max_slots, nr);
	if (ret < 0)
		return ret;

#ifdef DEBUG
	if (ret == 0)
		fprintf(stderr, "LOWPASS: couldn't read data\n");

	printf("LOWPASS: read %d samples (mem: %d nr x %d slots)\n",
		ret, nr, max_slots);
#endif

	if (!var->last_mt || !var->ideal_mt || max_slots > var->slots) {
		free(var->last_mt);
		free(var->ideal_mt);
		free(var->flags_mt);

		var->last_mt = calloc(max_slots, sizeof(struct ts_sample_mt));
		if (!var->last_mt)
			return -ENOMEM;

		var->ideal_mt = calloc(max_slots, sizeof(struct ts_sample_mt));
		if (!var->ideal_mt)
			return -ENOMEM;

		var->flags_mt = calloc(max_slots, sizeof(int));
		if (!var->flags_mt)
			return -ENOMEM;

		var->slots = max_slots;
	}

	for (i = 0; i < ret; i++) {
		for (j = 0; j < max_slots; j++) {
			if (!(samp[i][j].valid & TSLIB_MT_VALID))
				continue;

			if (samp[i][j].pressure == 0) {
				var->flags_mt[j] |= VAR_PENUP;
				continue;
			} else if (var->flags_mt[j] & VAR_PENUP) {
				var->flags_mt[j] &= ~VAR_PENUP;
				var->last_mt[j] = samp[i][j];
				continue;
			} else {
				var->ideal_mt[j] = samp[i][j];

				var->ideal_mt[j].x = var->last_mt[j].x;
				delta = samp[i][j].x - var->last_mt[j].x;
				if (delta <= var->threshold &&
				    delta >= -var->threshold)
					delta = 0;

				delta *= var->factor;
				var->ideal_mt[j].x += delta;


				var->ideal_mt[j].y = var->last_mt[j].y;
				delta = samp[i][j].y - var->last_mt[j].y;
				if (delta <= var->threshold &&
				    delta >= -var->threshold)
					delta = 0;

				delta *= var->factor;
				var->ideal_mt[j].y += delta;


				var->last_mt[j] = var->ideal_mt[j];
				samp[i][j] = var->ideal_mt[j];
			}
		}
	}

	return ret;
}

static int lowpass_fini(struct tslib_module_info *info)
{
	struct tslib_lowpass *var = (struct tslib_lowpass *)info;

	free(var->last_mt);
	free(var->ideal_mt);
	free(var->flags_mt);

	free(info);

	return 0;
}

static const struct tslib_ops lowpass_ops = {
	.read		= lowpass_read,
	.read_mt	= lowpass_read_mt,
	.fini		= lowpass_fini,
};

static int lowpass_factor(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_lowpass *var = (struct tslib_lowpass *)inf;
	long double v;
	int err = errno;

	v = strtod(str, NULL);

	if (v > 1 || v < 0)
		return -1;

	errno = err;
	switch ((int)(intptr_t)data) {
	case 1:
		var->factor = v;
#ifdef DEBUG
		printf("LOWPASS: factor is now %Lf\n", v);
#endif
		break;

	default:
		return -1;
	}
	return 0;
}

static int lowpass_threshold(struct tslib_module_info *inf, char *str,
			     void *data)
{
	struct tslib_lowpass *var = (struct tslib_lowpass *)inf;
	long result;
	int err = errno;

	result = strtol(str, NULL, 0);
	if (errno == ERANGE)
		return -1;

	errno = err;

	switch ((int)(intptr_t)data) {
	case 1:
		var->threshold = result;
#ifdef DEBUG
		printf("LOWPASS: threshold is now %ld\n", result);
#endif
		break;
	default:
		return -1;
	}
	return 0;
}

static const struct tslib_vars lowpass_vars[] = {
	{ "factor",	(void *)1, lowpass_factor },
	{ "threshold",  (void *)1, lowpass_threshold },
};

#define NR_VARS (sizeof(lowpass_vars) / sizeof(lowpass_vars[0]))

TSAPI struct tslib_module_info *lowpass_mod_init(__attribute__ ((unused)) struct tsdev *dev,
						 const char *params)
{
	struct tslib_lowpass *var;

	var = malloc(sizeof(struct tslib_lowpass));
	if (var == NULL)
		return NULL;

	memset(var, 0, sizeof(*var));
	var->module.ops = &lowpass_ops;

	var->factor = 0.4;
	var->threshold = 2;
	var->flags = VAR_PENUP;
	var->slots = 0;
	var->last_mt = NULL;
	var->ideal_mt = NULL;
	var->flags_mt = NULL;

	if (tslib_parse_vars(&var->module, lowpass_vars, NR_VARS, params)) {
		free(var);
		return NULL;
	}


	return &var->module;
}

#ifndef TSLIB_STATIC_LOWPASS_MODULE
	TSLIB_MODULE_INIT(lowpass_mod_init);
#endif
