#ifndef _TSLIB_FILTER_H_
#define _TSLIB_FILTER_H_
/*
 *  tslib/src/tslib-filter.h
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.
 *
 *
 * Internal touch screen library definitions.
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <tslib.h>

struct tslib_module_info;
struct tsdev;

struct tslib_vars {
	const char *name;
	void *data;
	int (*fn)(struct tslib_module_info *inf, char *str, void *data);
};

struct tslib_ops {
	int (*read)(struct tslib_module_info *inf, struct ts_sample *samp, int nr);
	int (*fini)(struct tslib_module_info *inf);
};

struct tslib_module_info {
	struct tsdev *dev;
	struct tslib_module_info *next;	/* next module in chain	*/
	void *handle;			/* dl handle		*/
	const struct tslib_ops *ops;
};

typedef struct tslib_module_info *(*tslib_module_init)(struct tsdev *dev, const char *params);
#define TSLIB_MODULE_INIT(f) TSAPI tslib_module_init mod_init = &f

TSAPI extern int tslib_parse_vars(struct tslib_module_info *,
			    const struct tslib_vars *, int,
			    const char *);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _TSLIB_FILTER_H_ */
