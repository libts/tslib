#ifndef _TSLIB_PRIVATE_H_
#define _TSLIB_PRIVATE_H_
/*
 *  tslib/src/tslib-private.h
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.
 *
 * $Id: tslib-private.h,v 1.2 2002/06/17 17:21:43 dlowder Exp $
 *
 * Internal touch screen library definitions.
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "tslib.h"
#include "tslib-filter.h"

struct tsdev {
	int fd;
	struct tslib_module_info *list;
};

int __ts_attach(struct tsdev *ts, struct tslib_module_info *info);
int ts_load_module(struct tsdev *dev, const char *module, const char *params);
int ts_error(const char *fmt, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _TSLIB_PRIVATE_H_ */
