/*
 *  tslib/plugins/linear.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: linear.c,v 1.10 2005/02/26 01:47:23 kergoth Exp $
 *
 * Linearly scale touchscreen values
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

struct tslib_linear {
	struct tslib_module_info module;
	int	swap_xy;

// Linear scaling and offset parameters for pressure
	int	p_offset;
	int	p_mult;
	int	p_div;

// Linear scaling and offset parameters for x,y (can include rotation)
	int	a[7];
};

static int
linear_read(struct tslib_module_info *info, struct ts_sample *samp, int nr)
{
	struct tslib_linear *lin = (struct tslib_linear *)info;
	int ret;
	int xtemp,ytemp;

	ret = info->next->ops->read(info->next, samp, nr);
	if (ret >= 0) {
		int nr;

		for (nr = 0; nr < ret; nr++, samp++) {
#ifdef DEBUG
			fprintf(stderr,"BEFORE CALIB--------------------> %d %d %d\n",samp->x, samp->y, samp->pressure);
#endif /*DEBUG*/
			xtemp = samp->x; ytemp = samp->y;
			samp->x = 	( lin->a[2] +
					lin->a[0]*xtemp + 
					lin->a[1]*ytemp ) / lin->a[6];
			samp->y =	( lin->a[5] +
					lin->a[3]*xtemp +
					lin->a[4]*ytemp ) / lin->a[6];

			samp->pressure = ((samp->pressure + lin->p_offset)
						 * lin->p_mult) / lin->p_div;
			if (lin->swap_xy) {
				int tmp = samp->x;
				samp->x = samp->y;
				samp->y = tmp;
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

static const struct tslib_ops linear_ops =
{
	.read	= linear_read,
	.fini	= linear_fini,
};

static int linear_xyswap(struct tslib_module_info *inf, char *str, void *data)
{
	struct tslib_linear *lin = (struct tslib_linear *)inf;

	lin->swap_xy = 1;
	return 0;
}

static const struct tslib_vars linear_vars[] =
{
	{ "xyswap",	(void *)1, linear_xyswap }
};

#define NR_VARS (sizeof(linear_vars) / sizeof(linear_vars[0]))

TSAPI struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{

	struct tslib_linear *lin;
	struct stat sbuf;
	int pcal_fd;
	char pcalbuf[200];
	int index;
	char *tokptr;
	char *calfile=NULL;
	char *defaultcalfile = "/etc/pointercal";

	lin = malloc(sizeof(struct tslib_linear));
	if (lin == NULL)
		return NULL;

	lin->module.ops = &linear_ops;

// Use default values that leave ts numbers unchanged after transform
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

	/*
	 * Check calibration file
	 */
	if( (calfile = getenv("TSLIB_CALIBFILE")) == NULL) calfile = defaultcalfile;
	if(stat(calfile,&sbuf)==0) {
		pcal_fd = open(calfile,O_RDONLY);
		read(pcal_fd,pcalbuf,200);
		lin->a[0] = atoi(strtok(pcalbuf," "));
		index=1;
		while(index<7) {
			tokptr = strtok(NULL," ");
			if(*tokptr!='\0') {
				lin->a[index] = atoi(tokptr);
				index++;
			}
		}
#ifdef DEBUG
		printf("Linear calibration constants: ");
		for(index=0;index<7;index++) printf("%d ",lin->a[index]);
		printf("\n");
#endif /*DEBUG*/
		close(pcal_fd);
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
