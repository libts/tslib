/*
 *  tslib/src/ts_config.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_config.c,v 1.1.1.1 2001/12/22 21:12:06 rmk Exp $
 *
 * Read the configuration and load the appropriate drivers.
 */
#include "config.h"
#include <stdio.h>
#include <string.h>

#include "tslib-private.h"

struct opt {
	const char *str;
	int (*fn)(struct tsdev *ts, char *rest);
};

static int ts_opt_module(struct tsdev *ts, char *rest)
{
	char *tok = strsep(&rest, " \t");

	return ts_load_module(ts, tok, rest);
}

static struct opt options[] = {
	{ "module", ts_opt_module },
};

#define NR_OPTS (sizeof(options) / sizeof(options[0]))

int ts_config(struct tsdev *ts)
{
	char buf[80], *p;
	FILE *f;
	int line = 0, ret = 0;

	f = fopen(TS_CONF, "r");
	if (!f)
		return -1;

	while ((p = fgets(buf, sizeof(buf), f)) != NULL && ret == 0) {
		struct opt *opt;
		char *e, *tok;

		line++;

		/*
		 * Did we read a whole line?
		 */
		e = strchr(p, '\n');
		if (!e) {
			ts_error("%d: line too long", line);
			break;
		}

		/*
		 * Chomp.
		 */
		*e = '\0';

		tok = strsep(&p, " \t");

		/*
		 * Ignore comments or blank lines.
		 */
		if (!tok || *tok == '#')
			continue;

		/*
		 * Search for the option.
		 */
		for (opt = options; opt < options + NR_OPTS; opt++)
			if (strcasecmp(tok, opt->str) == 0) {
				ret = opt->fn(ts, p);
				break;
			}

		if (opt == options + NR_OPTS) {
			ts_error("%d: option `%s' not recognised", line, tok);
			ret = -1;
		}
	}

	fclose(f);

	return ret;
}
