/*
 *  tslib/src/ts_close.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Close a touchscreen device.
 */
#include "config.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIBDL
#include <dlfcn.h>
#endif

#include "tslib-private.h"

void (*ts_close_restricted)(int fd, void *user_data) = NULL;

int ts_close(struct tsdev *ts)
{
	void *handle;
	int ret = 0;
	struct tslib_module_info *info, *next;

	info = ts->list;
	while (info) {
		/* Save the "next" pointer now because info will be freed */
		next = info->next;

		handle = info->handle;

		if (info->ops->fini)
			info->ops->fini(info);
		else
			free(info);
	#ifdef HAVE_LIBDL
		if (handle)
			dlclose(handle);
	#endif

		info = next;
	}

	if (ts_close_restricted)
		ts_close_restricted(ts->fd, NULL);
	else
		ret = close(ts->fd);

	free(ts->eventpath);

	free(ts);

	return ret;
}
