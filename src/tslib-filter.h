/*
 *  tslib/src/tslib-filter.h
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.
 *
 * $Id: tslib-filter.h,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Internal touch screen library definitions.
 */
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

extern int tslib_parse_vars(struct tslib_module_info *,
			    const struct tslib_vars *, int,
			    const char *);
