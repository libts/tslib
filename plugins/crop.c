/*
 *  tslib/plugins/crop.c
 *
 *  Copyright (C) 2024 Martin Kepplinger-Novaković
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>

#include "config.h"
#include "tslib-private.h"

struct tslib_crop {
	struct tslib_module_info module;
	int32_t *last_tid;
	uint32_t last_pressure;
	int	a[7];
	/* fb res from calibration-time */
	int32_t cal_res_x;
	int32_t cal_res_y;
	uint32_t rot;
};

static int crop_read(struct tslib_module_info *info, struct ts_sample *samp,
		     int nr)
{
	struct tslib_crop *crop = (struct tslib_crop *)info;
	int ret;
	int nread = 0;

	while (nread < nr) {
		struct ts_sample cur;

		ret = info->next->ops->read(info->next, &cur, 1);
		if (ret < 0)
			return ret;

		if (cur.x >= crop->cal_res_x ||
		    cur.x < 0 ||
		    cur.y >= crop->cal_res_y ||
		    cur.y < 0) {
			if (cur.pressure == 0) {
				if (crop->last_pressure == 0)
					continue;
			} else {
				continue;
			}
		}

		samp[nread++] = cur;
		crop->last_pressure = cur.pressure;
	}

	return nread;
}

static int crop_read_mt(struct tslib_module_info *info,
		       struct ts_sample_mt **samp, int max_slots, int nr)
{
	struct tslib_crop *crop = (struct tslib_crop *)info;
	int32_t ret;
	int32_t i, j;

	if (!info->next->ops->read_mt)
		return -ENOSYS;

	ret = info->next->ops->read_mt(info->next, samp, max_slots, nr);
	if (ret < 0)
		return ret;

	if (!crop->last_tid) {
		free(crop->last_tid);

		crop->last_tid = calloc(max_slots, sizeof(int32_t));
		if (!crop->last_tid)
			return -ENOMEM;
	}

	for (i = 0; i < ret; i++) {
		for (j = 0; j < max_slots; j++) {
			if (!(samp[i][j].valid & TSLIB_MT_VALID))
				continue;

			/* assume the input device uses 0..(fb-1) value. */

			if (samp[i][j].x >= crop->cal_res_x ||
			    samp[i][j].x < 0 ||
			    samp[i][j].y >= crop->cal_res_y ||
			    samp[i][j].y < 0) {
				if (samp[i][j].tracking_id == -1) {
					/*
					 * don't drop except last seen tid also -1
					 * otherwise, tid would get filtered
					 * out and new x/y would reach the app
					 */
					if (crop->last_tid[j] == -1) {
						samp[i][j].valid &= ~TSLIB_MT_VALID;
					}
				} else {
					/* drop */
					samp[i][j].valid &= ~TSLIB_MT_VALID;
				}
			}

			/* save the last not-dropped tid value */
			if (samp[i][j].valid & TSLIB_MT_VALID)
				crop->last_tid[j] = samp[i][j].tracking_id;
		}
	}

	return ret;
}

static int crop_fini(struct tslib_module_info *info)
{
	struct tslib_crop *crop = (struct tslib_crop *)info;

	free(crop->last_tid);
	free(info);

	return 0;
}

static const struct tslib_ops crop_ops = {
	.read		= crop_read,
	.read_mt	= crop_read_mt,
	.fini		= crop_fini,
};

TSAPI struct tslib_module_info *crop_mod_init(__attribute__ ((unused)) struct tsdev *dev,
					      __attribute__ ((unused)) const char *params)
{
	struct tslib_crop *crop;
	struct stat sbuf;
	FILE *pcal_fd;
	int index;
	char *calfile;

	crop = malloc(sizeof(struct tslib_crop));
	if (crop == NULL)
		return NULL;

	memset(crop, 0, sizeof(struct tslib_crop));
	crop->module.ops = &crop_ops;

	crop->last_tid = NULL;

	/*
	 * Get resolution from calibration file
	 */
	if ((calfile = getenv("TSLIB_CALIBFILE")) == NULL)
		calfile = TS_POINTERCAL;

	if (stat(calfile, &sbuf) == 0) {
		pcal_fd = fopen(calfile, "r");
		if (!pcal_fd) {
			free(crop);
			perror("fopen");
			return NULL;
		}

		for (index = 0; index < 7; index++)
			if (fscanf(pcal_fd, "%d", &crop->a[index]) != 1)
				break;

		if (!fscanf(pcal_fd, "%d %d",
			    &crop->cal_res_x, &crop->cal_res_y)) {
			fprintf(stderr,
				"CROP: Couldn't read resolution values\n");
		}

		if (!fscanf(pcal_fd, "%d", &crop->rot)) {
			fprintf(stderr, "CROP: Couldn't read rotation value\n");
		}

		fclose(pcal_fd);
	}
	return &crop->module;
}

#ifndef TSLIB_STATIC_CROP_MODULE
	TSLIB_MODULE_INIT(crop_mod_init);
#endif
