#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "tslib-private.h"

struct collie_ts_event { /* Used in the Sharp Zaurus SL-5000d and SL-5500 */
	long y;
	long x;
	long pressure;
	long long millisecs;
};

static int collie_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
	struct collie_ts_event *collie_evt;
	int ret;
	int total = 0;
	collie_evt = alloca(sizeof(*collie_evt) * nr);
	ret = read(ts->fd, collie_evt, sizeof(*collie_evt) * nr);
	if(ret > 0) {
		int nr = ret / sizeof(*collie_evt);
		while(ret >= (int)sizeof(*collie_evt)) {
			samp->x = collie_evt->x;
			samp->y = collie_evt->y;
			samp->pressure = collie_evt->pressure;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
			samp->tv.tv_usec = (collie_evt->millisecs % 1000) * 1000;
			samp->tv.tv_sec = collie_evt->millisecs / 1000;
			samp++;
			collie_evt++;
			ret -= sizeof(*collie_evt);
		}
	} else {
		return -1;
	}

	ret = nr;
	return ret;
}

static const struct tslib_ops collie_ops =
{
	.read	= collie_read,
};

TSAPI struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &collie_ops;
	return m;
}
