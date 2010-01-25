#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "tslib-private.h"

struct mk712_ts_event { /* Used in the Hitachi Webpad */
	unsigned int header;
	unsigned int x;
	unsigned int y;
	unsigned int reserved;
};

static int mk712_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
	struct mk712_ts_event *mk712_evt;
	int ret;
	int total = 0;
	mk712_evt = alloca(sizeof(*mk712_evt) * nr);
	ret = read(ts->fd, mk712_evt, sizeof(*mk712_evt) * nr);
	if(ret > 0) {
		int nr = ret / sizeof(*mk712_evt);
		while(ret >= (int)sizeof(*mk712_evt)) {
			samp->x = (short)mk712_evt->x;
			samp->y = (short)mk712_evt->y;
			if(mk712_evt->header==0)
				samp->pressure=1;
			else
				samp->pressure=0;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
			gettimeofday(&samp->tv,NULL);
			samp++;
			mk712_evt++;
			ret -= sizeof(*mk712_evt);
		}
	} else {
		return -1;
	}

	ret = nr;
	return ret;
}

static const struct tslib_ops mk712_ops =
{
	.read	= mk712_read,
};

TSAPI struct tslib_module_info *mk712_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &mk712_ops;
	return m;
}

#ifndef TSLIB_STATIC_MK712_MODULE
	TSLIB_MODULE_INIT(mk712_mod_init);
#endif
