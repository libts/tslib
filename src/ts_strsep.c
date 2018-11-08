/*
 *  tslib/src/ts_strsep.c
 *
 * Copyright (C) 2017 Martin Kepplinger <martink@posteo.de>
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 */
#include "ts_strsep.h"

char *ts_strsep(char **stringp, const char *delim)
{
	char *start = *stringp;
	char *p;

	p = (start) ? strpbrk(start, delim) : NULL;

	if (!p) {
		*stringp = NULL;
	} else {
		*p = '\0';
		*stringp = p + 1;
	}

	return start;
}
