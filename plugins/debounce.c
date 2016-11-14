/*
 *  tslib/plugins/debounce.c
 *
 *  Copyright (C) 2016 Martin Kepplinger <martin.kepplinger@ginzinger.com>
 *  Copyright (C) 2013 Melchior FRANZ <melchior.franz@ginzinger.com>
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
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

struct tslib_debounce {
	struct tslib_module_info	module;
	unsigned int			drop_threshold; // ms
	unsigned long long		last_release;
	int				last_pressure;
};

static int debounce_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_debounce *p = (struct tslib_debounce *)info;
	struct ts_sample *s;
	int ret;
	int num = 0;
	int i;
	unsigned long long now;
	long dt;
	__attribute__ ((unused)) enum { DOWN, MOVE, UP } mode;
	int drop = 0;
	int left;

	ret = info->next->ops->read(info->next, samp, nr);
	if (ret < 0)
		return ret;

	for (s = samp, i = 0; i < ret; i++, s++) {
		now = s->tv.tv_sec * 1e6 + s->tv.tv_usec;
		dt = (long)(now - p->last_release) / 1000; // [ms]
		mode = MOVE;

		if (!s->pressure) {
			mode = UP;
			p->last_release = now;
		} else if (!p->last_pressure) {
			mode = DOWN;
		}

		p->last_pressure = s->pressure;

		if (s->pressure) {
			if (dt >= 0 && dt < p->drop_threshold)
				drop = 1;
		}

#ifdef DEBUG
		if (drop)
			fprintf(stderr, "\033[%smtslib/debounce:\033[m  press=%u  x=%d  y=%d  dt=%ld%s\n",
					mode == DOWN ? "92" : mode == MOVE ? "32" : "93",
					s->pressure, s->x, s->y, dt,
					drop ? "  \033[31mdropped\033[m" : "");
#endif

		if (s->pressure && drop) {
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

static int debounce_fini(struct tslib_module_info *info)
{
	free(info);

	return 0;
}

static const struct tslib_ops debounce_ops =
{
	.read = debounce_read,
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


static const struct tslib_vars debounce_vars[] =
{
	{ "drop_threshold", (void *)0, read_debounce_vars }, // [ms]
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

	if (tslib_parse_vars(&p->module, debounce_vars, NR_VARS, params)) {
		free(p);
		return NULL;
	}

#ifdef DEBUG
	fprintf(stderr, "tslib/debounce: drop_threshold=%u\n",
				p->drop_threshold);
#endif

	return &p->module;
}

#ifndef TSLIB_STATIC_DEBOUNCE_MODULE
	TSLIB_MODULE_INIT(debounce_mod_init);
#endif
