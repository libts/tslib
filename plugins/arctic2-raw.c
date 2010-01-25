#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "tslib-private.h"

struct arctic2_ts_event { /* Used in the IBM Arctic II */
	signed short pressure;
	signed int x;
	signed int y;
	int millisecs;
	int flags;
};

static int arctic2_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
	struct arctic2_ts_event *arctic2_evt;
	int ret;
	int total = 0;
	arctic2_evt = alloca(sizeof(*arctic2_evt) * nr);
	ret = read(ts->fd, arctic2_evt, sizeof(*arctic2_evt) * nr);
	if(ret > 0) {
		int nr = ret / sizeof(*arctic2_evt);
		while(ret >= (int)sizeof(*arctic2_evt)) {
			samp->x = (short)arctic2_evt->x;
			samp->y = (short)arctic2_evt->y;
			samp->pressure = arctic2_evt->pressure;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
			gettimeofday(&samp->tv,NULL);
			samp++;
			arctic2_evt++;
			ret -= sizeof(*arctic2_evt);
		}
	} else {
		return -1;
	}

	ret = nr;
	return ret;
}

static const struct tslib_ops arctic2_ops =
{
	.read	= arctic2_read,
};

TSAPI struct tslib_module_info *arctic2_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &arctic2_ops;
	return m;
}

#ifndef TSLIB_STATIC_ARCTIC2_MODULE
	TSLIB_MODULE_INIT(arctic2_mod_init);
#endif
