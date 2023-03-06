/*
 *  tslib/plugins/linear.c
 *
 *  Copyright (C) 2016 Martin Kepplinger <martin.kepplinger@ginzinger.com>
 *  Copyright (C) 2005 Alberto Mardegan <mardy@sourceforge.net>
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Linearly scale touchscreen values
 */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include "config.h"
#include "tslib-private.h"
#include "tslib-filter.h"

struct tslib_linear {
	struct tslib_module_info module;
	int	swap_xy;

	/* Linear scaling and offset parameters for pressure */
	int	p_offset;
	int	p_mult;
	int	p_div;

	/* Linear scaling and offset parameters for x,y (can include rotation) */
	int	a[7];

	/* Screen resolution at the time when calibration was performed */
	unsigned int cal_res_x;
	unsigned int cal_res_y;

	/* Rotation. Forced or from calibration time */
	unsigned int rot;
};

#ifdef PLUGIN_LINEAR_LIMIT_TO_CALIBRATION_RESOLUTION
int min(int a, int b)
{
  return a < b ? a : b;
}

int max(int a, int b)
{
  return a > b ? a : b;
}
#endif

static int linear_read(struct tslib_module_info *info, struct ts_sample *samp,
		       int nr_samples)
{
	struct tslib_linear *lin = (struct tslib_linear *)info;
	int ret;
	int xtemp, ytemp;

	ret = info->next->ops->read(info->next, samp, nr_samples);
	if (ret >= 0) {
		int nr;

		for (nr = 0; nr < ret; nr++, samp++) {
		#ifdef DEBUG
			fprintf(stderr,
				"BEFORE CALIB--------------------> %d %d %d\n",
				samp->x, samp->y, samp->pressure);
		#endif /* DEBUG */
			xtemp = samp->x; ytemp = samp->y;
			samp->x =	(lin->a[2] +
					lin->a[0]*xtemp +
					lin->a[1]*ytemp) / lin->a[6];
			samp->y =	(lin->a[5] +
					lin->a[3]*xtemp +
					lin->a[4]*ytemp) / lin->a[6];
			if (info->dev->res_x && lin->cal_res_x)
				samp->x = samp->x * info->dev->res_x
					  / lin->cal_res_x;
			if (info->dev->res_y && lin->cal_res_y)
				samp->y = samp->y * info->dev->res_y
					  / lin->cal_res_y;

			samp->pressure = ((samp->pressure + lin->p_offset)
					  * lin->p_mult) / lin->p_div;
			if (lin->swap_xy) {
				int tmp = samp->x;

				samp->x = samp->y;
				samp->y = tmp;
			}

			switch (lin->rot) {
			int rot_tmp;
			case 0:
				break;
			case 1:
				rot_tmp = samp->x;
				samp->x = samp->y;
				samp->y = lin->cal_res_x - rot_tmp - 1;
				break;
			case 2:
				samp->x = lin->cal_res_x - samp->x - 1;
				samp->y = lin->cal_res_y - samp->y - 1;
				break;
			case 3:
				rot_tmp = samp->x;
				samp->x = lin->cal_res_y - samp->y - 1;
				samp->y = rot_tmp ;
				break;
			default:
				break;
			}

              #ifdef PLUGIN_LINEAR_LIMIT_TO_CALIBRATION_RESOLUTION
                  samp->x = max(samp->x, 0);
                  samp->y = max(samp->y, 0);
                  if(lin->cal_res_x)
                  {
                    samp->x = min(samp->x, lin->cal_res_x - 1);
                  }

                  if(lin->cal_res_y)
                  {
                    samp->y = min(samp->y, lin->cal_res_y - 1);
                  }
              #endif
		}
	}

	return ret;
}

static int linear_read_mt(struct tslib_module_info *info,
			  struct ts_sample_mt **samp,
			  int max_slots, int nr_samples)
{
	struct tslib_linear *lin = (struct tslib_linear *)info;
	int ret;
	int xtemp, ytemp;
	int i;
	int nr;

	if (!info->next->ops->read_mt)
		return -ENOSYS;

	ret = info->next->ops->read_mt(info->next, samp, max_slots, nr_samples);
	if (ret < 0)
		return ret;

	for (nr = 0; nr < ret; nr++) {
	#ifdef DEBUG
		printf("LINEAR:   read %d samples (mem: %d nr x %d slots)\n",
		       ret, nr_samples, max_slots);
		fprintf(stderr, "BEFORE CALIB:\n");
	#endif /*DEBUG*/
		for (i = 0; i < max_slots; i++) {
			if (!(samp[nr][i].valid & TSLIB_MT_VALID))
				continue;

		#ifdef DEBUG
			fprintf(stderr, "          (slot %d) X:%4d Y:%4d P:%d\n",
				i, samp[nr][i].x, samp[nr][i].y,
				samp[nr][i].pressure);
		#endif /*DEBUG*/
			xtemp = samp[nr][i].x;
			ytemp = samp[nr][i].y;
			samp[nr][i].x =	(lin->a[2] +
					lin->a[0]*xtemp +
					lin->a[1]*ytemp) / lin->a[6];
			samp[nr][i].y =	(lin->a[5] +
					lin->a[3]*xtemp +
					lin->a[4]*ytemp) / lin->a[6];
			if (info->dev->res_x && lin->cal_res_x)
				samp[nr][i].x = samp[nr][i].x *
						info->dev->res_x /
						lin->cal_res_x;
			if (info->dev->res_y && lin->cal_res_y)
				samp[nr][i].y = samp[nr][i].y *
						info->dev->res_y /
						lin->cal_res_y;

			samp[nr][i].pressure = ((samp[nr][i].pressure +
						 lin->p_offset) *
						 lin->p_mult) /
						 lin->p_div;
			if (lin->swap_xy) {
				int tmp = samp[nr][i].x;

				samp[nr][i].x = samp[nr][i].y;
				samp[nr][i].y = tmp;
			}

			switch (lin->rot) {
			int rot_tmp;
			case 0:
				break;
			case 1:
				rot_tmp = samp[nr][i].x;
				samp[nr][i].x = samp[nr][i].y;
				samp[nr][i].y = lin->cal_res_x - rot_tmp -1 ;
				break;
			case 2:
				samp[nr][i].x = lin->cal_res_x - samp[nr][i].x - 1;
				samp[nr][i].y = lin->cal_res_y - samp[nr][i].y - 1;
				break;
			case 3:
				rot_tmp = samp[nr][i].x;
				samp[nr][i].x = lin->cal_res_y - samp[nr][i].y - 1;
				samp[nr][i].y = rot_tmp ;
				break;
			default:
				break;
			}
		}
	}

	return ret;
}

static int linear_fini(struct tslib_module_info *info)
{
	free(info);
	return 0;
}

static const struct tslib_ops linear_ops = {
	.read		= linear_read,
	.read_mt	= linear_read_mt,
	.fini		= linear_fini,
};

static int linear_xyswap(struct tslib_module_info *inf,
			 __attribute__ ((unused)) char *str,
			 __attribute__ ((unused)) void *data)
{
	struct tslib_linear *lin = (struct tslib_linear *)inf;

	lin->swap_xy = 1;
	return 0;
}

static int linear_p_offset(struct tslib_module_info *inf, char *str,
			   __attribute__ ((unused)) void *data)
{
	struct tslib_linear *lin = (struct tslib_linear *)inf;

	unsigned long offset = strtoul(str, NULL, 0);

	if (offset == ULONG_MAX && errno == ERANGE)
		return -1;

	lin->p_offset = offset;
	return 0;
}

static int linear_p_mult(struct tslib_module_info *inf, char *str,
			 __attribute__ ((unused)) void *data)
{
	struct tslib_linear *lin = (struct tslib_linear *)inf;
	unsigned long mult = strtoul(str, NULL, 0);

	if (mult == ULONG_MAX && errno == ERANGE)
		return -1;

	lin->p_mult = mult;
	return 0;
}

static int linear_p_div(struct tslib_module_info *inf, char *str,
			__attribute__ ((unused)) void *data)
{
	struct tslib_linear *lin = (struct tslib_linear *)inf;
	unsigned long div = strtoul(str, NULL, 0);

	if (div == ULONG_MAX && errno == ERANGE)
		return -1;

	lin->p_div = div;
	return 0;
}

static int linear_rot(struct tslib_module_info *inf, char *str,
			__attribute__ ((unused)) void *data)
{
	struct tslib_linear *lin = (struct tslib_linear *)inf;
	unsigned long rot = strtoul(str, NULL, 0);

	if (rot == ULONG_MAX && errno == ERANGE)
		return -1;

	switch (rot) {
	case 0:
	case 1:
	case 2:
	case 3:
		lin->rot = rot;
		break;
	default:
		return -1;
	}

	return 0;
}

static const struct tslib_vars linear_vars[] = {
	{ "xyswap",	(void *)1, linear_xyswap },
	{ "pressure_offset", NULL, linear_p_offset},
	{ "pressure_mul", NULL, linear_p_mult},
	{ "pressure_div", NULL, linear_p_div},
	{ "rot", NULL, linear_rot},
};

#define NR_VARS (sizeof(linear_vars) / sizeof(linear_vars[0]))

TSAPI struct tslib_module_info *linear_mod_init(__attribute__ ((unused)) struct tsdev *dev,
						const char *params)
{

	struct tslib_linear *lin;
	struct stat sbuf;
	FILE *pcal_fd;
	int index;
	char *calfile;

	lin = malloc(sizeof(struct tslib_linear));
	if (lin == NULL)
		return NULL;

	lin->module.ops = &linear_ops;

	/* Use default values that leave ts numbers unchanged after transform */
	lin->a[0] = 1;
	lin->a[1] = 0;
	lin->a[2] = 0;
	lin->a[3] = 0;
	lin->a[4] = 1;
	lin->a[5] = 0;
	lin->a[6] = 1;
	lin->p_offset = 0;
	lin->p_mult   = 1;
	lin->p_div    = 1;
	lin->swap_xy  = 0;
	lin->rot = 0;
	lin->cal_res_x = 0;
	lin->cal_res_y = 0;

	/*
	 * Check calibration file
	 */
	if ((calfile = getenv("TSLIB_CALIBFILE")) == NULL)
		calfile = TS_POINTERCAL;

	if (stat(calfile, &sbuf) == 0) {
		pcal_fd = fopen(calfile, "r");
		if (!pcal_fd) {
			free(lin);
			perror("fopen");
			return NULL;
		}

		for (index = 0; index < 7; index++)
			if (fscanf(pcal_fd, "%d", &lin->a[index]) != 1)
				break;

		if (!fscanf(pcal_fd, "%d %d", &lin->cal_res_x, &lin->cal_res_y))
			fprintf(stderr,
				"LINEAR: Couldn't read resolution values\n");

		if (!fscanf(pcal_fd, "%d", &lin->rot)) {
#ifdef DEBUG
			printf("LINEAR: Couldn't read rotation value\n");
#endif
		} else {
#ifdef DEBUG
			printf("LINEAR: Reading rotation %d from calibfile\n",
				lin->rot);
#endif
		}

#ifdef DEBUG
		printf("Linear calibration constants: ");
		for (index = 0; index < 7; index++)
			printf("%d ", lin->a[index]);
		printf("\n");
#endif /*DEBUG*/
		fclose(pcal_fd);
	}

	/*
	 * Parse the parameters.
	 */
	if (tslib_parse_vars(&lin->module, linear_vars, NR_VARS, params)) {
		free(lin);
		return NULL;
	}

	return &lin->module;
}

#ifndef TSLIB_STATIC_LINEAR_MODULE
	TSLIB_MODULE_INIT(linear_mod_init);
#endif
