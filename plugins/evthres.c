/*
 *  tslib/plugins/evthres.c
 *
 *  Copyright (C) 2019 Martin Kepplinger <martin.kepplinger@ginzinger.com>
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Threshold filter to drop a tap sequence if too short
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

#define EVTHRES_SIZE_MAX	500
#define EVTHRES_SIZE_DEFAULT	5

struct evthres {
	struct tslib_module_info	module;
	unsigned int			size;
	struct ts_sample		*buf;
	unsigned int			full;
	unsigned int			filling_mode;
	int				slots;
	struct ts_sample_mt		**buf_mt;
	unsigned int			*full_mt;
	unsigned int			*filling_mode_mt;
};


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

static int evthres_read(struct tslib_module_info *info, struct ts_sample *samp,
			int nr)
{
	struct evthres *c = (struct evthres *)info;
	struct ts_sample *s;
	int ret, i;
	int count = 0;

	/* if buffer is full, empty it before reading new samples */
	for (i = 0; i < nr; i++) {
		if (!c->filling_mode && c->full > 0) {
			memcpy(samp, &c->buf[0], sizeof(struct ts_sample));
			memmove(&c->buf[0],
				&c->buf[1],
				(c->size - 1) * sizeof(c->buf[0]));
			memset(&c->buf[c->size - 1], 0, sizeof(struct ts_sample));
			c->full--;
		#ifdef DEBUG
			printf("EVTHRES: emptying buffer\n");
		#endif
			printsample("EVTHRES: ", samp);
			count++;
		} else {
			samp->pressure = 0;
		}
	}
	if (count)
		return count;

	if (!info->next->ops->read)
		return -ENOSYS;

	ret = info->next->ops->read(info->next, samp, nr);
	if (ret <= 0)
		return ret;

	for (s = samp; ret > 0; s++, ret--) {
		if (c->filling_mode == 0) {
			printsample("EVTHRES: ", samp);

			if (!samp->pressure)
				c->filling_mode = 1;

			count++;
			continue;
		}

		/* pen up: drop all */
		if ((!s->pressure) && (c->full < c->size)) {
			c->full = 0;
			memset(c->buf, 0,
			       c->size * sizeof(c->buf[0]));
			continue;
		}

		/* accept one sample to buf */
		memmove(c->buf,
			&c->buf[1],
			(c->size - 1) * sizeof(c->buf[0]));
		memcpy(&c->buf[c->size - 1], samp, sizeof(c->buf[0]));
		c->full++;

		if (c->full < c->size) {
			c->filling_mode = 1;
		#ifdef DEBUG
			printf("EVTHRES: filling buffer\n");
		#endif
		} else {
			c->filling_mode = 0;
		#ifdef DEBUG
			printf("EVTHRES: buffer full\n");
		#endif
		}
	}

	return count;
}

static int evthres_read_mt(struct tslib_module_info *inf,
			   struct ts_sample_mt **samp, int max_slots, int nr)
{
	struct evthres *c = (struct evthres *)inf;
	int ret;
	int i, j;
	int count_nr = 0;
	int count = 0;


	/* if buffer is full, empty it before reading new samples */
	for (i = 0; i < nr; i++) {
		count = 0;
		for (j = 0; j < max_slots; j++) {
			if (c->filling_mode_mt && c->filling_mode_mt[j] == 0 &&
			    c->full_mt && c->full_mt[j] > 0) {
				samp[i][j] = c->buf_mt[j][0];
				memmove(&c->buf_mt[j][0],
					&c->buf_mt[j][1],
					(c->size - 1) * sizeof(c->buf_mt[j][0]));
				memset(&c->buf_mt[j][c->size - 1], 0, sizeof(struct ts_sample_mt));
				c->full_mt[j]--;
			#ifdef DEBUG
				printf("EVTHRES slot %d: emptying buffer\n", j);
			#endif
				printsample_mt("EVTHRES: ", &samp[i][j]);
				count++;

				if (!samp[i][j].pressure) {
		printf("                    INTERNAL ERROR\n");
				}
			} else {
				samp[i][j].valid &= ~TSLIB_MT_VALID;
			}
		}
		if (count)
			count_nr++;
	}
	if (count_nr) {
	#ifdef DEBUG
		printf("EVTHRES: %d samples from buffer (mem: %d nr x %d slots)\n",
		       count_nr, nr, max_slots);
	#endif
		return count_nr;
	}

	/* buffer is empty. read new samples */
	if (!inf->next->ops->read_mt)
		return -ENOSYS;

	ret = inf->next->ops->read_mt(inf->next, samp, max_slots, nr);
	if (ret < 0)
		return ret;

#ifdef DEBUG
	if (ret == 0)
		fprintf(stderr, "EVTHRES: couldn't read data\n");

	printf("EVTHRES: read %d samples (mem: %d nr x %d slots)\n",
	       ret, nr, max_slots);
#endif

	if (c->buf_mt == NULL || max_slots > c->slots) {
		if (c->buf_mt) {
			for (i = 0; i < c->slots; i++)
				free(c->buf_mt[i]);

			free(c->buf_mt);
		}

		c->buf_mt = malloc(max_slots * sizeof(struct ts_sample_mt *));
		if (!c->buf_mt)
			return -ENOMEM;

		for (i = 0; i < max_slots; i++) {
			c->buf_mt[i] = calloc(c->size,
					      sizeof(struct ts_sample_mt));
			if (!c->buf_mt[i]) {
				for (j = 0; j < i; j++)
					free(c->buf_mt[j]);

				free(c->buf_mt);
				c->buf_mt = NULL;

				return -ENOMEM;
			}
		}

		c->slots = max_slots;
	}

	if (c->full_mt == NULL || max_slots > c->slots) {
		free(c->full_mt);

		c->full_mt = calloc(max_slots, sizeof(unsigned int));
		if (!c->full_mt)
			return -ENOMEM;
	}

	if (c->filling_mode_mt == NULL || max_slots > c->slots) {
		free(c->filling_mode_mt);

		c->filling_mode_mt = calloc(max_slots, sizeof(unsigned int));
		if (!c->filling_mode_mt)
			return -ENOMEM;

		for (i = 0; i < max_slots; i++)
			c->filling_mode_mt[i] = 1;
	}

	for (i = 0; i < ret; i++) {
		for (j = 0; j < max_slots; j++) {
			if (!(samp[i][j].valid & TSLIB_MT_VALID))
				continue;

			/* if filling == 0 : return sample */
			if (c->filling_mode_mt[j] == 0) {

			#ifdef DEBUG
				printf("EVTHRES slot %d: direct pass through\n", j);
			#endif
				printsample_mt("EVTHRES: ", &samp[i][j]);

				if (!samp[i][j].pressure)
					c->filling_mode_mt[j] = 1;

				continue;
			}

			/* pen up: drop all */
			if ((!samp[i][j].pressure) && (c->full_mt[j] < c->size)) {
				c->full_mt[j] = 0;
				c->filling_mode_mt[j] = 1;
				memset(&c->buf_mt[j][0], 0,
					c->size * sizeof(c->buf_mt[j][0]));
			#ifdef DEBUG
				printf("EVTHRES: pen up: DROP the sequence\n");
			#endif
				samp[i][j].valid &= ~TSLIB_MT_VALID;
				continue;
			}

			/* accept one sample to buf */
			memmove(&c->buf_mt[j][0],
				&c->buf_mt[j][1],
				(c->size - 1) * sizeof(c->buf_mt[j][0]));
			c->buf_mt[j][c->size - 1] = samp[i][j];
			c->full_mt[j]++;

			if (c->full_mt[j] < c->size) {
				c->filling_mode_mt[j] = 1;
			#ifdef DEBUG
				printf("EVTHRES slot %d: filling buffer\n", j);
			#endif
			} else {
				c->filling_mode_mt[j] = 0;
			#ifdef DEBUG
				printf("EVTHRES slot %d: buffer full\n", j);
			#endif
			}

			samp[i][j].valid &= ~TSLIB_MT_VALID;
		}
	}

	return ret;
}

static int evthres_fini(struct tslib_module_info *inf)
{
	struct evthres *c = (struct evthres *) inf;
	int i;

	for (i = 0; i < c->slots; i++)
		free(c->buf_mt[i]);

	free(c->buf_mt);

	free(inf);

	return 0;
}

static const struct tslib_ops evthres_ops = {
	.read		= evthres_read,
	.read_mt	= evthres_read_mt,
	.fini		= evthres_fini,
};

static int evthres_size(struct tslib_module_info *inf, char *str,
			__attribute__ ((unused)) void *data)
{
	struct evthres *m = (struct evthres *)inf;
	unsigned long n;
	int err = errno;
	unsigned int max = EVTHRES_SIZE_MAX;

	n = strtoul(str, NULL, 0);

	if (n >= max) {
		fprintf(stderr, "EVTHRES: size exceeds maximum of %d\n", max);
		return -1;
	}

	errno = err;
	m->buf = malloc(sizeof(struct ts_sample) * n);
	m->size = n;

	return 0;
}

static const struct tslib_vars evthres_vars[] = {
	{ "N", (void *)1, evthres_size },
};

#define NR_VARS (sizeof(evthres_vars) / sizeof(evthres_vars[0]))

TSAPI struct tslib_module_info *evthres_mod_init(__attribute__ ((unused)) struct tsdev *dev,
						 const char *params)
{
	struct evthres *c;

	c = malloc(sizeof(struct evthres));
	if (c == NULL)
		return NULL;

	memset(c, 0, sizeof(struct evthres));
	c->module.ops = &evthres_ops;

	c->buf = NULL;
	c->buf_mt = NULL;
	c->slots = 0;
	c->filling_mode = 1;

	if (tslib_parse_vars(&c->module, evthres_vars, NR_VARS, params)) {
		free(c);
		return NULL;
	}

	if (c->buf == NULL) {
		c->buf = malloc(sizeof(struct ts_sample) * EVTHRES_SIZE_DEFAULT);
		c->size = EVTHRES_SIZE_DEFAULT;
	#ifdef DEBUG
		printf("Using default size of %d\n", c->size);
	#endif
	}

	return &(c->module);
}

#ifndef TSLIB_STATIC_EVTHRES_MODULE
	TSLIB_MODULE_INIT(evthres_mod_init);
#endif
