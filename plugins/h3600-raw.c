#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "tslib-private.h"

struct h3600_ts_event { /* Used in the Compaq IPAQ */
	unsigned short pressure;
	unsigned short x;
	unsigned short y;
	unsigned short pad;
};

static int h3600_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
	struct h3600_ts_event *h3600_evt;
	int ret;
	int total = 0;
	h3600_evt = alloca(sizeof(*h3600_evt) * nr);
	ret = read(ts->fd, h3600_evt, sizeof(*h3600_evt) * nr);
	if(ret > 0) {
		int nr = ret / sizeof(*h3600_evt);
		while(ret >= (int)sizeof(*h3600_evt)) {
			samp->x = h3600_evt->x;
			samp->y = h3600_evt->y;
			samp->pressure = h3600_evt->pressure;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
			gettimeofday(&samp->tv,NULL);
			samp++;
			h3600_evt++;
			ret -= sizeof(*h3600_evt);
		}
	} else {
		return -1;
	}

	ret = nr;
	return ret;
}

static const struct tslib_ops h3600_ops =
{
	.read	= h3600_read,
};

TSAPI struct tslib_module_info *h3600_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &h3600_ops;
	return m;
}

#ifndef TSLIB_STATIC_H3600_MODULE
	TSLIB_MODULE_INIT(h3600_mod_init);
#endif
