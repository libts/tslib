#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "tslib-private.h"

struct ucb1x00_ts_event  {   /* Used in UCB1x00 style touchscreens */
	unsigned short pressure;
	unsigned short x;
	unsigned short y;
	unsigned short pad;
	struct timeval stamp;
};

static int ucb1x00_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
	struct ucb1x00_ts_event *ucb1x00_evt;
	int ret;
	int total = 0;
	ucb1x00_evt = alloca(sizeof(*ucb1x00_evt) * nr);
	ret = read(ts->fd, ucb1x00_evt, sizeof(*ucb1x00_evt) * nr);
	if(ret > 0) {
		int nr = ret / sizeof(*ucb1x00_evt);
		while(ret >= (int)sizeof(*ucb1x00_evt)) {
			samp->x = ucb1x00_evt->x;
			samp->y = ucb1x00_evt->y;
			samp->pressure = ucb1x00_evt->pressure;
#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
			samp->tv.tv_usec = ucb1x00_evt->stamp.tv_usec;
			samp->tv.tv_sec = ucb1x00_evt->stamp.tv_sec;
			samp++;
			ucb1x00_evt++;
			ret -= sizeof(*ucb1x00_evt);
		}
	} else {
		return -1;
	}

	ret = nr;
	return ret;
}

static const struct tslib_ops ucb1x00_ops =
{
	.read	= ucb1x00_read,
};

TSAPI struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &ucb1x00_ops;
	return m;
}
