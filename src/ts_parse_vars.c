/*
 *  tslib/src/ts_parse_vars.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Parse modules' parameters and call their own parsing functions
 */
#include "config.h"

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <string.h>

#include "tslib-private.h"

#if !defined HAVE_STRSEP
#include "ts_strsep.h"
#endif

#define BUF_SIZE 1024

static char s_holder[BUF_SIZE];

int tslib_parse_vars(struct tslib_module_info *mod,
		     const struct tslib_vars *vars, int nr,
		     const char *str)
{
	char *s, *p;
	int ret = 0;

	if (!str)
		return 0;

	strncpy(s_holder, str, BUF_SIZE - 1);
	s_holder[BUF_SIZE - 1] = '\0';

	s = s_holder;
#if !defined HAVE_STRSEP
	while ((p = ts_strsep(&s, " \t")) != NULL && ret == 0) {
#else
	while ((p = strsep(&s, " \t")) != NULL && ret == 0) {
#endif
		const struct tslib_vars *v;
		char *eq;

		eq = strchr(p, '=');
		if (eq)
			*eq++ = '\0';

		for (v = vars; v < vars + nr; v++)
			if (strcasecmp(v->name, p) == 0) {
				ret = v->fn(mod, eq, v->data);
				break;
			}
	}

	return ret;
}
