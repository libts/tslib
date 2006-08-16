#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "tslib-private.h"

struct corgi_ts_event { /* Used in the Sharp Zaurus SL-C700 */
	short pressure;
	short x;
	short y;
	short millisecs;
};

static int corgi_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
	struct corgi_ts_event *corgi_evt;
	int ret;
	int total = 0;
	corgi_evt = alloca(sizeof(*corgi_evt) * nr);
	ret = read(ts->fd, corgi_evt, sizeof(*corgi_evt) * nr);
	if(ret > 0) {
		int nr = ret / sizeof(*corgi_evt);
		while(ret >= (int)sizeof(*corgi_evt)) {
			samp->x = corgi_evt->x;
			samp->y = corgi_evt->y;
			samp->pressure = corgi_evt->pressure;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
			samp->tv.tv_usec = (corgi_evt->millisecs % 1000) * 1000;
			samp->tv.tv_sec = corgi_evt->millisecs / 1000;
			samp++;
			corgi_evt++;
			ret -= sizeof(*corgi_evt);
		}
	} else {
		return -1;
	}
	ret = nr;
	return ret;
}

static const struct tslib_ops corgi_ops =
{
	.read	= corgi_read,
};

TSAPI struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &corgi_ops;
	return m;
}
