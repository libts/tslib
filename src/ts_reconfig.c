/*
 *  tslib/src/ts_reconfig.c
 *    based on tslib/src/ts_close.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Reloads all modules - useful to reload calibration data.
 */
#include "config.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <dlfcn.h>

#include "tslib-private.h"

int ts_reconfig(struct tsdev *ts)
{
	void *handle;
	int ret;
	struct tslib_module_info *info, *next;
	int fd;
	
	info = ts->list;
	while(info) {
		/* Save the "next" pointer now because info will be freed */
		next = info->next;
		
		handle = info->handle;
		info->ops->fini(info);
		if (handle)
			dlclose(handle);
		
		info = next;
	}

	fd = ts->fd;	/* save temp */
	memset(ts, 0, sizeof(struct tsdev));
	ts->fd = fd;

	ret = ts_config(ts);
	return ret;
}
