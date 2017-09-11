/*
 *  tslib/src/ts_config_filter.c
 *
 *  Copyright (C) 2017 Martin Kepplinger.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Interface for editing ts.conf. This edits "module" filters, not
 * "module_raw" access plugins.
 */
#include <stdlib.h>
#include <stdio.h>
#include "tslib-private.h"

/* return a list of structs. Each struct is one commented-in module */
struct ts_module_conf *ts_conf_get(struct tsdev *ts, const char *path)
{
	/* config file read */
	return NULL;
}

int ts_conf_set(struct tsdev *ts, const char *path, struct ts_module_conf *conf)
{
	return 0;
}
