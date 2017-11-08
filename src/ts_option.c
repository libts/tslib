/*
 *  tslib/src/ts_option.c
 *
 *  Copyright (C) 2005 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Interface for setting parameters for the core library
 */
#include "config.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tslib-private.h"

int ts_option(struct tsdev *ts, enum ts_param param, ...)
{
	int ret;
	va_list ap;

	va_start(ap, param);
	ret = 0;

	switch (param) {
	case TS_SCREEN_RES:
		ts->res_x = va_arg(ap, unsigned int);
		ts->res_y = va_arg(ap, unsigned int);
		break;
	case TS_SCREEN_ROT:
		ts->rotation = va_arg(ap, int);
		break;
	}
	va_end(ap);

	return ret;
}

