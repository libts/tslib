/*
 *  tslib/src/tslib-private.h
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.
 *
 * $Id: tslib-private.h,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Internal touch screen library definitions.
 */
#include "tslib.h"
#include "tslib-filter.h"

struct tsdev {
	int fd;
	struct tslib_module_info *list;
};

int __ts_attach(struct tsdev *ts, struct tslib_module_info *info);
int ts_load_module(struct tsdev *dev, const char *module, const char *params);
int ts_error(const char *fmt, ...);
