/*
 *  tslib/plugins/linear-h2200.c
 *
 *  Copyright (C) 2004 Michael Opdenacker
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: linear-h2200.c,v 1.1 2005/02/26 01:54:51 kergoth Exp $
 *
 * Linearly scale touchscreen values for HP iPAQ h22xx.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>

#include "tslib.h"
#include "tslib-filter.h"

struct tslib_linear_h2200 {
        struct tslib_module_info module;
};

/*

  Thanks to Lau Norgaard <lau@robo.dk> for the formula!

  [u v] = [1  x  y  x*y  x2  y2] * P

  P = [ 14.0274374997464	-10.4143500663246
	0.963183808844262	0.123820939383483
       -0.0175631972840528	0.90783932803656
	3.01072646091237e-005	-0.00022066295637918
	1.78550793439434e-005	5.26174785439132e-006
	1.24328256232492e-006	0.000171150736110672]

  Using fixed point arithmetics as ARM processors don't have floating point
  capabilities. Otherwise, using floating point would cause time consuming
  kernel exceptions. With our input and output data, found that we could
  use a 12.20 format, provided we use extra intermediate shifts with very
  small numbers and products are done in a careful order that doesn't
  yield big intermediate products).

*/

#define M20(x,y) ((long)(((long long)x * (long long)y) >> 20))
#define M32(x,y) ((long)(((long long)x * (long long)y) >> 32))

static int
linear_h2200_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	int ret;
        long x, y, new_x, new_y;

	ret = info->next->ops->read(info->next, samp, nr);
	if (ret >= 0) {
		int nr;

		for (nr = 0; nr < ret; nr++, samp++) {

			x = ((long) samp->x) << 20;
			y = ((long) samp->y) << 20;

			/* Caution: constants have been multiplied by 2^20
			  (to save runtime). Some of them have been
			  multiplied by 2^32 when they were too small.
			  An extra >>12 is then needed.

			  Note: we never multiply x*y or y*y first
			  (intermediate result too big, could overflow),
			  we multiply by the constant first. Because of this,
			  we can't reuse x^2, y^2 and x*y
			*/

			new_x = 14708834 + M20(1009971,x) + M20(-18416,y) +
				M20(M32(129310,x),y) + M20(M32(76687,x),x) +
				M20(M32(5340,y),y);

			new_y = -10920238 + M20(129836,x) + M20(951939,y) +
				M20(M32(-947740,x),y) + M20(M32(22599,x),x) +
				M20(M32(735087,y),y);

			samp->x = (int) (new_x >> 20);    
			samp->y = (int) (new_y >> 20);    
		}
	}

	return ret;
}

static int linear_h2200_fini(struct tslib_module_info *info)
{
	free(info);
	return 0;
}

static const struct tslib_ops linear_h2200_ops =
{
	.read	= linear_h2200_read,
	.fini	= linear_h2200_fini,
};

TSAPI struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{

	struct tslib_linear_h2200 *lin;

	lin = malloc(sizeof(struct tslib_linear_h2200));
	if (lin == NULL)
		return NULL;

	lin->module.ops = &linear_h2200_ops;

	return &lin->module;
}
