/*
 *  tslib/src/ts_error.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_error.c,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Error handling for tslib.
 */
#include "config.h"
#include <stdarg.h>
#include <stdio.h>

static int stderrfn(const char *fmt, va_list ap)
{
	return vfprintf(stderr, fmt, ap);
}

/*
 * Change this hook to point to your custom error handling function.
 */
int (*ts_error_fn)(const char *fmt, va_list ap) = stderrfn;

int ts_error(const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = ts_error_fn(fmt, ap);
	va_end(ap);

	return ret;
}
