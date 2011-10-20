/*
 *  tslib/src/ts_close.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Close a touchscreen device.
 */
#include "config.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <dlfcn.h>

#include "tslib-private.h"

int ts_close(struct tsdev *ts)
{
	void *handle;
	int ret;
	struct tslib_module_info *info, *prev;

	for(info = ts->list, prev = info;
	    info != NULL;
	    info = prev->next, prev = info) {
		handle = info->handle;
		info->ops->fini(info);
		if (handle)
			dlclose(handle);
	}

	ret = close(ts->fd);
	free(ts);

	return ret;
}
