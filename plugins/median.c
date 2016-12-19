/*
 *  tslib/plugins/median.c
 *
 *  Copyright (C) 2009 Marel ehf
 *  Author Kári Davíðsson
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * Median filter incomming data. For some theory, see
 * https://en.wikipedia.org/wiki/Median_filter
 */

#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "tslib.h"
#include "tslib-filter.h"

#define MEDIAN_DEPTH_MAX 128

#define PREPARESAMPLE( array, context, member ) { \
	int count = context->size; \
	while( count-- ) { \
		array[count] = context->delay[count].member; \
	}\
}

#define PREPARESAMPLE_MT( array, context, member, slot) { \
	int count = context->size; \
	while( count-- ) { \
		array[count] = context->delay_mt[slot][count].member; \
	}\
}

struct median_context {
	struct tslib_module_info	module;
	int				size;
	struct ts_sample		*delay;
	struct ts_sample_mt		**delay_mt;
	int				withsamples;
	int				*withsamples_mt;
	int				slots;
	unsigned int			depth;
};

static int comp_int(const void * n1, const void * n2)
{
	int * i1 = (int *) n1;
	int * i2 = (int *) n2;

	return  *i1 < *i2 ? -1 : 1;
}

static int comp_uint(const void * n1, const void * n2)
{
	unsigned int * i1 = (unsigned int *) n1;
	unsigned int * i2 = (unsigned int *) n2;

	return  *i1 < *i2 ? -1 : 1;
}

static void printsamples(__attribute__ ((unused)) char *prefix,
			 __attribute__ ((unused)) int *samples,
			 __attribute__ ((unused)) size_t count )
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

static void printsample(__attribute__ ((unused)) char * prefix,
			__attribute__ ((unused)) struct ts_sample * s)
{
#ifdef DEBUG
	printf( "%s using Point at (%d,%d) with pressure %u\n",
		prefix, s->x, s->y, s->pressure);
#endif
}

static void printsample_mt(__attribute__ ((unused)) char *prefix,
			   __attribute__ ((unused)) struct ts_sample_mt *s)
{
#ifdef DEBUG
	printf( "%s (slot %d) using Point at (%d,%d) with pressure %u\n",
		prefix, s->slot, s->x, s->y, s->pressure);
#endif
}

static int median_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct median_context *c = (struct median_context *)inf;
	int ret;

	ret = inf->next->ops->read(inf->next, samp, nr);
	if (ret > 0) {
		int i;
		struct ts_sample * s;

		for (s = samp, i = 0; i < ret; i++, s++) {
			int sorted[c->size];
			unsigned int usorted[c->size];
			unsigned int cpress;

			cpress = s->pressure;

			memmove(&c->delay[0],
				&c->delay[1],
				(c->size - 1) * sizeof(c->delay[0]));
			c->delay[c->size -1] = *s;

			PREPARESAMPLE( sorted, c, x );
			printsamples("MEDIAN: X Before", sorted, c->size );
			qsort( &sorted[0], c->size, sizeof( sorted[0] ), comp_int);
			s->x = sorted[c->size / 2];
			printsamples("MEDIAN: X After", sorted, c->size );

			PREPARESAMPLE( sorted, c, y );
			printsamples("MEDIAN: Y Before", sorted, c->size );
			qsort( &sorted[0], c->size, sizeof( sorted[0] ), comp_int);
			s->y = sorted[c->size / 2];
			printsamples("MEDIAN: Y After", sorted, c->size );

			PREPARESAMPLE( usorted, c, pressure );
			printsamples("MEDIAN: Pressure Before", (int *)usorted, c->size );
			qsort( &usorted[0], c->size, sizeof( usorted[0] ),comp_uint);
			s->pressure = usorted[ c->size / 2];
			printsamples("MEDIAN: Pressure After", (int *)usorted, c->size );

			printsample("", s );

			if ((cpress == 0)  && (c->withsamples != 0)) { /* We have penup */
				/* Flush the line we now must wait for c->size / 2
				   samples untill we get valid data again */
				memset(c->delay, 0, sizeof( struct ts_sample) * c->size);
				c->withsamples = 0;
			#ifdef DEBUG
				printf("MEDIAN: Pen Up\n");
			#endif
				s->pressure = cpress;
			} else if ((cpress != 0) && (c->withsamples == 0) ) { /* We have pen down */
				c->withsamples = 1;
			#ifdef DEBUG
				printf("MEDIAN: Pen Down\n");
			#endif
			}
		}
	}

	return ret;
}

static int median_read_mt(struct tslib_module_info *inf, struct ts_sample_mt **samp, int max_slots, int nr)
{
	struct median_context *c = (struct median_context *)inf;
	int ret;
	int i, j;

	ret = inf->next->ops->read_mt(inf->next, samp, max_slots, nr);
	if (ret < 0)
		return ret;

#ifdef DEBUG
	if (ret == 0)
		fprintf(stderr, "MEDIAN: couldn't read data\n");

	printf("MEDIAN: read %d samples (mem: %d nr x %d slots)\n", ret, nr, max_slots);
#endif

	if (c->delay_mt == NULL || max_slots > c->slots) {
		if (c->delay_mt) {
			for (i = 0; i < c->slots; i++) {
				if (c->delay_mt[i])
					free(c->delay_mt[i]);
			}
			free(c->delay_mt);
		}

		c->delay_mt = malloc(max_slots * sizeof(struct ts_sample_mt *));
		if (!c->delay_mt)
			return -ENOMEM;

		for (i = 0; i < max_slots; i++) {
			c->delay_mt[i] = calloc(c->size, sizeof(struct ts_sample_mt));
			if (!c->delay_mt[i]) {
				for (j = 0; j <= i; j++) {
					if(c->delay_mt[j])
						free(c->delay_mt[j]);
				}
				if (c->delay_mt)
					free(c->delay_mt);

				return -ENOMEM;
			}
		}

		c->slots = max_slots;
	}

	if (c->withsamples_mt == NULL || max_slots > c->slots) {
		if (c->withsamples_mt)
			free(c->withsamples_mt);

		c->withsamples_mt = calloc(max_slots, sizeof(int));
		if (!c->withsamples_mt)
			return -ENOMEM;
	}

	for (i = 0; i < ret; i++) {
		for (j = 0; j < max_slots; j++) {
			int sorted[c->size];
			unsigned int usorted[c->size];
			unsigned int cpress = 0;

			if (samp[i][j].valid != 1)
				continue;

			memset(sorted, 0, c->size * sizeof(int));
			memset(usorted, 0, c->size * sizeof(unsigned int));

			cpress = samp[i][j].pressure;

			memmove(&c->delay_mt[j][0],
				&c->delay_mt[j][1],
				(c->size - 1) * sizeof(c->delay_mt[j][0]));
			c->delay_mt[j][c->size - 1] = samp[i][j];

			PREPARESAMPLE_MT(sorted, c, x, j);
			printsamples_mt("MEDIAN: X Before", sorted, c->size, j);
			qsort( &sorted[0], c->size, sizeof( sorted[0] ), comp_int);
			samp[i][j].x = sorted[c->size / 2];
			printsamples_mt("MEDIAN: X After", sorted, c->size, j);

			PREPARESAMPLE_MT( sorted, c, y, j);
			printsamples_mt("MEDIAN: Y Before", sorted, c->size, j);
			qsort( &sorted[0], c->size, sizeof( sorted[0] ), comp_int);
			samp[i][j].y = sorted[c->size / 2];
			printsamples_mt("MEDIAN: Y After", sorted, c->size, j);

			PREPARESAMPLE_MT( usorted, c, pressure, j);
			printsamples_mt("MEDIAN: Pressure Before", (int *)usorted, c->size, j);
			qsort( &usorted[0], c->size, sizeof( usorted[0] ),comp_uint);
			samp[i][j].pressure = usorted[ c->size / 2];
			printsamples_mt("MEDIAN: Pressure After", (int *)usorted, c->size, j);

			printsample_mt("MEDIAN: ", &samp[i][j]);

			if ((cpress == 0)  && (c->withsamples_mt[j] != 0)) { /* We have penup */
				/* Flush the line we now must wait for c->size / 2
				   samples untill we get valid data again */
				memset(c->delay_mt[j], 0, sizeof( struct ts_sample) * c->size);
				c->withsamples_mt[j] = 0;
			#ifdef DEBUG
				printf("MEDIAN: Pen Up\n");
			#endif
				samp[i][j].pressure = cpress;
			} else if ((cpress != 0) && (c->withsamples_mt[j] == 0) ) { /* We have pen down */
				c->withsamples_mt[j] = 1;
			#ifdef DEBUG
				printf("MEDIAN: Pen Down\n");
			#endif
			}
		}
	}

	return ret;
}

static int median_fini(struct tslib_module_info *inf)
{
	struct median_context * c = ( struct median_context *) inf;
	int i;

	free(c->delay);

	for (i = 0; i < c->slots; i++) {
		if (c->delay_mt[i])
			free(c->delay_mt[i]);
	}

	if (c->delay_mt)
		free(c->delay_mt);

	if (c->withsamples_mt)
		free(c->withsamples_mt);

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
	m->delay = malloc( sizeof( struct ts_sample ) * v );
	m->size = v;

	return 0;
}

static const struct tslib_vars raw_vars[] =
{
	{ "depth", (void *)1, median_depth },
};

#define NR_VARS (sizeof(raw_vars) / sizeof(raw_vars[0]))

TSAPI struct tslib_module_info *median_mod_init(__attribute__ ((unused)) struct tsdev *dev,
						const char *params)
{
	struct median_context *c;

	c = malloc(sizeof(struct median_context));
	if (c == NULL)
		return NULL;

	memset( c, 0, sizeof( struct median_context ) );
	c->module.ops = &median_ops;

	c->withsamples_mt = NULL;
	c->delay_mt = NULL;
	c->slots = 0;

	if (tslib_parse_vars(&c->module, raw_vars, NR_VARS, params)) {
		free(c);
		return NULL;
	}

	if( c->delay == NULL ) {
		c->delay = malloc( sizeof( struct ts_sample ) * 3 );
		c->size = 3;
		printf("Using default size of 3\n");
	}

	return &(c->module);
}

#ifndef TSLIB_STATIC_MEDIAN_MODULE
	TSLIB_MODULE_INIT(median_mod_init);
#endif
