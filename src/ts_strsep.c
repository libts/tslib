/*
 *  tslib/src/ts_strsep.c
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 */
#include "ts_strsep.h"

char *ts_strsep(char **stringp, const char *delim)
{
	char *start = *stringp;
	char *p;

	p = (start != NULL) ? strpbrk(start, delim) : NULL;

	if (p == NULL) {
		*stringp = NULL;
	} else {
		*p = '\0';
		*stringp = p + 1;
	}

	return start;
}
