#ifndef _TSLIB_PRIVATE_H_
#define _TSLIB_PRIVATE_H_
/*
 *  tslib/src/tslib-private.h
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.
 *
 * $Id: tslib-private.h,v 1.3 2004/07/21 19:12:59 dlowder Exp $
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
	struct tslib_module_info *list_raw; /* points to position in 'list' where raw reads
					       come from.  default is the position of the
					       ts_read_raw module. */
};

int __ts_attach(struct tsdev *ts, struct tslib_module_info *info);
int __ts_attach_raw(struct tsdev *ts, struct tslib_module_info *info);
int ts_load_module(struct tsdev *dev, const char *module, const char *params);
int ts_load_module_raw(struct tsdev *dev, const char *module, const char *params);
int ts_error(const char *fmt, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _TSLIB_PRIVATE_H_ */
