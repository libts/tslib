#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "tslib-private.h"

struct tatung_ts_event { /* Tatung touchscreen 4bytes protocol */
	unsigned char x1;
	unsigned char x2;
	unsigned char y1;
	unsigned char y2;
};

static int tatung_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
	struct tatung_ts_event *tatung_evt;
	int ret;
	int total = 0;
	tatung_evt = alloca(sizeof(*tatung_evt) * nr);
	ret = read(ts->fd, tatung_evt, sizeof(*tatung_evt) * nr);
	if(ret > 0) {
		int nr = ret / sizeof(*tatung_evt);
		while(ret >= (int)sizeof(*tatung_evt)) {

			if (tatung_evt->x1==240 || tatung_evt->x2==240 || tatung_evt->y1==240 || tatung_evt->y2==240)
			{
				ret = nr;
				return ret;
			}

			samp->x = (tatung_evt->x1)*31 + (tatung_evt->x2)-64;
			samp->y = (tatung_evt->y1)*31 + (tatung_evt->y2)-192;
			samp->pressure=1;
			//samp->pressure = tatung_evt->pressure;

#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
			gettimeofday(&samp->tv,NULL);
			samp++;
			tatung_evt++;
			ret -= sizeof(*tatung_evt);
		}
	} else {
		return -1;
	}

	samp->pressure=0;
	ret = nr;
	return ret;
}

static const struct tslib_ops tatung_ops =
{
	.read	= tatung_read,
};

TSAPI struct tslib_module_info *tatung_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &tatung_ops;
	return m;
}

#ifndef TSLIB_STATIC_TATUNG_MODULE
	TSLIB_MODULE_INIT(tatung_mod_init);
#endif
