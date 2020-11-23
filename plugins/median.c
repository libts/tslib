/*
 *  tslib/plugins/median.c
 *
 *  Copyright (C) 2016 Martin Kepplinger <martin.kepplinger@ginzinger.com>
 *  Copyright (C) 2009 Marel ehf
 *  Author Kári Davíðsson
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Median filter incoming data. For some theory, see
 * https://en.wikipedia.org/wiki/Median_filter
 */

#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "config.h"
#include "tslib.h"
#include "tslib-filter.h"

#define MEDIAN_DEPTH_MAX 128

#define PREPARESAMPLE(array, context, member) { \
	int count = context->size; \
	while (count--) { \
		array[count] = context->delay[count].member; \
	} \
}

#define PREPARESAMPLE_MT(array, context, member, slot) { \
	int count = context->size; \
	while (count--) { \
		array[count] = context->delay_mt[slot][count].member; \
	} \
}

struct median_context {
	struct tslib_module_info	module;
	int				size;
	struct ts_sample		*delay;
	struct ts_sample_mt		**delay_mt;
	int				withsamples;
	int				*withsamples_mt;
	short				*pen_down;
	int				slots;
	unsigned int			depth;
	int32_t				*sorted;
	uint32_t			*usorted;
};

static int comp_int(const void *n1, const void *n2)
{
	int *i1 = (int *) n1;
	int *i2 = (int *) n2;

	return  *i1 < *i2 ? -1 : 1;
}

static int comp_uint(const void *n1, const void *n2)
{
	unsigned int *i1 = (unsigned int *) n1;
	unsigned int *i2 = (unsigned int *) n2;

	return  *i1 < *i2 ? -1 : 1;
}

static void printsamples(__attribute__ ((unused)) char *prefix,
			 __attribute__ ((unused)) int *samples,
			 __attribute__ ((unused)) size_t count)
{
#ifdef DEBUG
	size_t j;

	printf("%s Using %d samples ", prefix, (int) count);
	for (j = 0; j < count; j++)
		printf(" %d", samples[j]);

	printf("\n");
#endif
}

static void printsamples_mt(__attribute__ ((unused)) char *prefix,
			    __attribute__ ((unused)) int *samples,
			    __attribute__ ((unused)) size_t count,
			    __attribute__ ((unused)) int slot)
{
#ifdef DEBUG
	size_t j;

	printf("%s (slot %d) Using %d samples ", prefix, slot, (int) count);
	for (j = 0; j < count; j++)
		printf(" %d", samples[j]);

	printf("\n");
#endif
}

static void printsample(__attribute__ ((unused)) char *prefix,
			__attribute__ ((unused)) struct ts_sample *s)
{
#ifdef DEBUG
	printf("%s using Point at (%d,%d) with pressure %u\n",
	       prefix, s->x, s->y, s->pressure);
#endif
}

static void printsample_mt(__attribute__ ((unused)) char *prefix,
			   __attribute__ ((unused)) struct ts_sample_mt *s)
{
#ifdef DEBUG
	printf("%s (slot %d) using Point at (%d,%d) with pressure %u\n",
	       prefix, s->slot, s->x, s->y, s->pressure);
#endif
}

static int median_read(struct tslib_module_info *inf, struct ts_sample *samp,
		       int nr)
{
	struct median_context *c = (struct median_context *)inf;
	int ret;

	ret = inf->next->ops->read(inf->next, samp, nr);
	if (ret > 0) {
		int i;
		struct ts_sample *s;

		for (s = samp, i = 0; i < ret; i++, s++) {
			unsigned int cpress;

			cpress = s->pressure;

			memmove(&c->delay[0],
				&c->delay[1],
				(c->size - 1) * sizeof(c->delay[0]));
			c->delay[c->size - 1] = *s;

			PREPARESAMPLE(c->sorted, c, x);
			printsamples("MEDIAN: X Before", c->sorted, c->size);
			qsort(&c->sorted[0], c->size,
			      sizeof(c->sorted[0]),
			      comp_int);
			s->x = c->sorted[c->size / 2];
			printsamples("MEDIAN: X After ", c->sorted, c->size);

			PREPARESAMPLE(c->sorted, c, y);
			printsamples("MEDIAN: Y Before", c->sorted, c->size);
			qsort(&c->sorted[0], c->size,
			      sizeof(c->sorted[0]),
			      comp_int);
			s->y = c->sorted[c->size / 2];
			printsamples("MEDIAN: Y After ", c->sorted, c->size);

			PREPARESAMPLE(c->usorted, c, pressure);
			printsamples("MEDIAN: Pressure Before",
				     (int *)c->usorted, c->size);
			qsort(&c->usorted[0], c->size,
			      sizeof(c->usorted[0]),
			      comp_uint);
			s->pressure = c->usorted[c->size / 2];
			printsamples("MEDIAN: Pressure After ",
				     (int *)c->usorted, c->size);

			printsample("", s);

			if ((cpress == 0)  && (c->withsamples != 0)) {
				/* We have penup. Flush the line we now must
				 * wait for c->size / 2 samples until we get
				 * valid data again
				 */
				memset(c->delay,
				       0,
				       sizeof(struct ts_sample) * c->size);
				c->withsamples = 0;
			#ifdef DEBUG
				printf("MEDIAN: Pen Up\n");
			#endif
				s->pressure = cpress;
			} else if ((cpress != 0) && (c->withsamples == 0)) {
				/* We have pen down */
				c->withsamples = 1;
			#ifdef DEBUG
				printf("MEDIAN: Pen Down\n");
			#endif
			}
		}
	}

	return ret;
}

static int median_read_mt(struct tslib_module_info *inf,
			  struct ts_sample_mt **samp, int max_slots, int nr)
{
	struct median_context *c = (struct median_context *)inf;
	int ret;
	int i, j;

	if (!inf->next->ops->read_mt)
		return -ENOSYS;

	ret = inf->next->ops->read_mt(inf->next, samp, max_slots, nr);
	if (ret < 0)
		return ret;

#ifdef DEBUG
	if (ret == 0)
		fprintf(stderr, "MEDIAN: couldn't read data\n");

	printf("MEDIAN:   read %d samples (mem: %d nr x %d slots)\n",
	       ret, nr, max_slots);
#endif

	if (c->delay_mt == NULL || max_slots > c->slots) {
		if (c->delay_mt) {
			for (i = 0; i < c->slots; i++)
				free(c->delay_mt[i]);

			free(c->delay_mt);
		}

		c->delay_mt = malloc(max_slots * sizeof(struct ts_sample_mt *));
		if (!c->delay_mt)
			return -ENOMEM;

		for (i = 0; i < max_slots; i++) {
			c->delay_mt[i] = calloc(c->size,
						sizeof(struct ts_sample_mt));
			if (!c->delay_mt[i]) {
				for (j = 0; j < i; j++)
					free(c->delay_mt[j]);

				free(c->delay_mt);
				c->delay_mt = NULL;

				return -ENOMEM;
			}
		}

		c->slots = max_slots;
	}

	if (c->withsamples_mt == NULL || max_slots > c->slots) {
		free(c->withsamples_mt);

		c->withsamples_mt = calloc(max_slots, sizeof(int));
		if (!c->withsamples_mt)
			return -ENOMEM;
	}

	if (c->pen_down == NULL || max_slots > c->slots) {
		free(c->pen_down);

		c->pen_down = calloc(max_slots, sizeof(short));
		if (!c->pen_down)
			return -ENOMEM;
	}

	for (i = 0; i < ret; i++) {
		for (j = 0; j < max_slots; j++) {
			unsigned int cpress = 0;
			c->pen_down[j] = -1;

			if (!(samp[i][j].valid & TSLIB_MT_VALID))
				continue;

			memset(c->sorted, 0, c->size * sizeof(int32_t));
			memset(c->usorted, 0, c->size * sizeof(uint32_t));

			cpress = samp[i][j].pressure;

			memmove(&c->delay_mt[j][0],
				&c->delay_mt[j][1],
				(c->size - 1) * sizeof(c->delay_mt[j][0]));
			c->delay_mt[j][c->size - 1] = samp[i][j];

			PREPARESAMPLE_MT(c->sorted, c, x, j);
			printsamples_mt("MEDIAN: X Before", c->sorted, c->size, j);
			qsort(&c->sorted[0], c->size, sizeof(c->sorted[0]), comp_int);
			samp[i][j].x = c->sorted[c->size / 2];
			printsamples_mt("MEDIAN: X After ", c->sorted, c->size, j);

			PREPARESAMPLE_MT(c->sorted, c, y, j);
			printsamples_mt("MEDIAN: Y Before", c->sorted, c->size, j);
			qsort(&c->sorted[0], c->size, sizeof(c->sorted[0]), comp_int);
			samp[i][j].y = c->sorted[c->size / 2];
			printsamples_mt("MEDIAN: Y After ", c->sorted, c->size, j);

			PREPARESAMPLE_MT(c->usorted, c, pressure, j);
			printsamples_mt("MEDIAN: Pressure Before",
					(int *)c->usorted, c->size, j);
			qsort(&c->usorted[0], c->size, sizeof(c->usorted[0]),
			      comp_uint);
			samp[i][j].pressure = c->usorted[c->size / 2];
			printsamples_mt("MEDIAN: Pressure After ",
					(int *)c->usorted, c->size, j);

			printsample_mt("MEDIAN: ", &samp[i][j]);

			if ((cpress == 0) && (c->withsamples_mt[j] != 0)) {
				/* We have penup. Flush the line we now must
				 * wait for c->size / 2 samples until we get
				 * valid data again
				 */
				memset(c->delay_mt[j],
				       0,
				       sizeof(struct ts_sample_mt) * c->size);

				c->withsamples_mt[j] = 0;
			#ifdef DEBUG
				printf("MEDIAN: Pen Up\n");
			#endif
				samp[i][j].pressure = cpress;
				samp[i][j].pen_down = 0;
				c->pen_down[j] = -1;
			} else if ((cpress != 0) &&
				   (c->withsamples_mt[j] == 0)) {
				/* We have pen down */
				c->withsamples_mt[j] = 1;
			#ifdef DEBUG
				printf("MEDIAN: Pen Down\n");
			#endif
			}

			if (cpress != 0 && c->withsamples_mt[j] <= c->size / 2) {
				samp[i][j].valid = 0;
				c->withsamples_mt[j]++;
			} else if (cpress !=0 && c->pen_down[j] == -1 &&
				   c->withsamples_mt[j] > c->size / 2) {
				/* the pen-down we generate */
				c->pen_down[j] = 1;
				samp[i][j].pen_down = 1;
			}
		}
	}

	return ret;
}

static int median_fini(struct tslib_module_info *inf)
{
	struct median_context *c = (struct median_context *) inf;
	int i;

	free(c->delay);

	for (i = 0; i < c->slots; i++)
		free(c->delay_mt[i]);

	free(c->delay_mt);
	free(c->withsamples_mt);
	free(c->sorted);
	free(c->usorted);

	free(inf);

	return 0;
}

static const struct tslib_ops median_ops = {
	.read		= median_read,
	.read_mt	= median_read_mt,
	.fini		= median_fini,
};

static int median_depth(struct tslib_module_info *inf, char *str,
			__attribute__ ((unused)) void *data)
{
	struct median_context *m = (struct median_context *)inf;
	unsigned long v;
	int err = errno;
	unsigned int max = MEDIAN_DEPTH_MAX;

	v = strtoul(str, NULL, 0);

	if (v >= max) {
		fprintf(stderr, "MEDIAN: depth exceeds maximum of %d\n", max);
		return -1;
	}

	errno = err;
	m->delay = malloc(sizeof(struct ts_sample) * v);
	m->size = v;

	m->sorted = calloc(m->size, sizeof(int32_t));
	if (!m->sorted)
		return -1;

	m->usorted = calloc(m->size, sizeof(uint32_t));
	if (!m->usorted)
		return -1;

	return 0;
}

static const struct tslib_vars median_vars[] = {
	{ "depth", (void *)1, median_depth },
};

#define NR_VARS (sizeof(median_vars) / sizeof(median_vars[0]))

TSAPI struct tslib_module_info *median_mod_init(__attribute__ ((unused)) struct tsdev *dev,
						const char *params)
{
	struct median_context *c;

	c = malloc(sizeof(struct median_context));
	if (c == NULL)
		return NULL;

	memset(c, 0, sizeof(struct median_context));
	c->module.ops = &median_ops;

	c->withsamples_mt = NULL;
	c->delay_mt = NULL;
	c->slots = 0;
	c->sorted = NULL;
	c->usorted = NULL;

	if (tslib_parse_vars(&c->module, median_vars, NR_VARS, params)) {
		free(c);
		return NULL;
	}

	if (c->delay == NULL) {
		c->delay = malloc(sizeof(struct ts_sample) * 3);
		c->size = 3;
	#ifdef DEBUG
		printf("Using default size of 3\n");
	#endif
	}

	return &(c->module);
}

#ifndef TSLIB_STATIC_MEDIAN_MODULE
	TSLIB_MODULE_INIT(median_mod_init);
#endif
