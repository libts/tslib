/*
 *  tslib/plugins/debounce.c
 *
 *  Copyright (C) 2017 Martin Kepplinger <martin.kepplinger@ginzinger.com>
 *  Copyright (C) 2013 Melchior FRANZ <melchior.franz@ginzinger.com>
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Problem: An intended single-tap by the user should be understood as
 * such. Be it because of the touch device's properties or an unintended
 * wrong input by the user, multiple input events that lead to a "tap"
 * could occur.
 *
 * Solution: For a specified time after stopping to touch, we drop input
 * samples.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include "tslib.h"
#include "tslib-filter.h"

enum debounce_mode {
	DOWN,
	MOVE,
	UP
};

struct tslib_debounce {
	struct tslib_module_info	module;
	unsigned int			drop_threshold; /* ms */
	int64_t				last_release;
	int				last_pressure;
	int64_t				*last_release_mt;
	int				*last_pressure_mt;
	int32_t				current_max_slots;
	enum debounce_mode		*mode_mt;
};

static int debounce_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_debounce *p = (struct tslib_debounce *)info;
	struct ts_sample *s;
	int ret;
	int num = 0;
	int i;
	int64_t now;
	long dt;
	int drop = 0;
	int left;
	__attribute__ ((unused)) enum debounce_mode mode;

	ret = info->next->ops->read(info->next, samp, nr);
	if (ret < 0)
		return ret;

	for (s = samp, i = 0; i < ret; i++, s++) {
		now = s->tv.tv_sec * 1e6 + s->tv.tv_usec;
		dt = (long)(now - p->last_release) / 1000; /* ms */
		mode = MOVE;

		drop = 0;

		if (!s->pressure) {
			mode = UP;
			p->last_release = now;
		} else if (!p->last_pressure) {
			mode = DOWN;
		}

		p->last_pressure = s->pressure;

		if (dt < p->drop_threshold)
			drop = 1;

#ifdef DEBUG
		fprintf(stderr, "\033[%smDEBOUNCE:\033[m  press=%u  x=%d  y=%d  dt=%ld%s\n",
				mode == DOWN ? "92" : mode == MOVE ? "32" : "93",
				s->pressure, s->x, s->y, dt,
				drop ? "  \033[31mdropped\033[m" : "");
#endif

		if (drop) {
			left = ret - num - 1;
			if (left > 0) {
				memmove(s, s + 1, left * sizeof(struct ts_sample));
				s--;
				continue;
			}
			break;
		}

		num++;
	}
	return num;
}

static int debounce_read_mt(struct tslib_module_info *info, struct ts_sample_mt **samp,
			    int max_slots, int nr_samples)
{
	struct tslib_debounce *p = (struct tslib_debounce *)info;
	int ret;
	int64_t now;
	long dt;
	int nr;
	int i;

	if (p->mode_mt == NULL || max_slots > p->current_max_slots) {
		free(p->mode_mt);

		p->mode_mt = calloc(max_slots, sizeof(unsigned int));
		if (!p->mode_mt)
			return -ENOMEM;

		p->current_max_slots = max_slots;
	}

	if (p->last_release_mt == NULL || max_slots > p->current_max_slots) {
		free(p->last_release_mt);

		p->last_release_mt = calloc(max_slots, sizeof(int64_t));
		if (!p->last_release_mt)
			return -ENOMEM;

		p->current_max_slots = max_slots;
	}

	if (p->last_pressure_mt == NULL || max_slots > p->current_max_slots) {
		free(p->last_pressure_mt);

		p->last_pressure_mt = calloc(max_slots, sizeof(int));
		if (!p->last_pressure_mt)
			return -ENOMEM;

		p->current_max_slots = max_slots;
	}

	if (!info->next->ops->read_mt)
		return -ENOSYS;

	ret = info->next->ops->read_mt(info->next, samp, max_slots, nr_samples);
	if (ret < 0)
		return ret;

#ifdef DEBUG
	printf("DEBOUNCE: read %d samples (mem: %d nr x %d slots)\n", ret, nr_samples, max_slots);
#endif

	for (nr = 0; nr < ret; nr++) {
		for (i = 0; i < max_slots; i++) {
			if (!(samp[nr][i].valid & TSLIB_MT_VALID))
				continue;

			now = samp[nr][i].tv.tv_sec * 1e6 + samp[nr][i].tv.tv_usec;
			dt = (long)(now - p->last_release_mt[i]) / 1000; /* ms */
			p->mode_mt[i] = MOVE;

			if (!samp[nr][i].pressure) {
				p->mode_mt[i] = UP;
				p->last_release_mt[i] = now;
			} else if (!p->last_pressure_mt[i]) {
				p->mode_mt[i] = DOWN;
			}

			p->last_pressure_mt[i] = samp[nr][i].pressure;

			if (dt < p->drop_threshold)
				samp[nr][i].valid = 0;

	#ifdef DEBUG
			fprintf(stderr, "\033[%smDEBOUNCE:\033[m (slot %d) P:%u X:%4d  Y:%4d  dt=%ld%s\n",
					p->mode_mt[i] == DOWN ? "92" : p->mode_mt[i] == MOVE ? "32" : "93",
					samp[nr][i].slot, samp[nr][i].pressure,
					samp[nr][i].x, samp[nr][i].y, dt,
					samp[nr][i].valid ? "" : "  \033[31mdropped\033[m");
	#endif
		}
	}

	return nr;
}

static int debounce_fini(struct tslib_module_info *info)
{
	struct tslib_debounce *p = (struct tslib_debounce *)info;

	free(p->last_release_mt);
	free(p->last_pressure_mt);

	free(info);

	return 0;
}

static const struct tslib_ops debounce_ops = {
	.read = debounce_read,
	.read_mt = debounce_read_mt,
	.fini = debounce_fini,
};

static int read_debounce_vars(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_debounce *p = (struct tslib_debounce *)inf;
	unsigned long v;
	int err = errno;

	v = strtoul(str, NULL, 0);
	if (v == ULONG_MAX && errno == ERANGE)
		return -1;

	errno = err;
	switch ((int)(intptr_t)data) {
	case 0:
		p->drop_threshold = v;
		break;

	default:
		return -1;
	}
	return 0;
}


static const struct tslib_vars debounce_vars[] = {
	{ "drop_threshold", (void *)0, read_debounce_vars }, /* ms */
};

#define NR_VARS (sizeof(debounce_vars) / sizeof(debounce_vars[0]))

TSAPI struct tslib_module_info *debounce_mod_init(__attribute__ ((unused)) struct tsdev *dev,
						  const char *params)
{
	struct tslib_debounce *p;

	p = malloc(sizeof(struct tslib_debounce));
	if (p == NULL)
		return NULL;

	p->module.ops = &debounce_ops;
	p->drop_threshold = 0;
	p->last_release = 0ULL;
	p->last_pressure = 0;
	p->last_release_mt = NULL;
	p->last_pressure_mt = NULL;
	p->current_max_slots = 0;
	p->mode_mt = NULL;

	if (tslib_parse_vars(&p->module, debounce_vars, NR_VARS, params)) {
		free(p);
		return NULL;
	}

	return &p->module;
}

#ifndef TSLIB_STATIC_DEBOUNCE_MODULE
	TSLIB_MODULE_INIT(debounce_mod_init);
#endif
