/*
 *  tslib/src/ts_config.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Read the configuration and load the appropriate drivers.
 */
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tslib-private.h"

/* Maximum line size is BUF_SIZE - 2 
 * -1 for fgets and -1 to test end of line
 */
#define BUF_SIZE 512

int ts_config(struct tsdev *ts)
{
	char buf[BUF_SIZE], *p;
	FILE *f;
	int line = 0;
	int ret = 0;

	char *conffile;

	if( (conffile = getenv("TSLIB_CONFFILE")) == NULL) {
		conffile = strdup (TS_CONF);
	}

	f = fopen(conffile, "r");
	if (!f) {
		perror("Couldnt open tslib config file");
		return -1;
	}

	buf[BUF_SIZE - 2] = '\0';
	while ((p = fgets(buf, BUF_SIZE, f)) != NULL) {
		char *e;
		char *tok;
		char *module_name;

		line++;

		/* Chomp */
		e = strchr(p, '\n');
		if (e) {
			*e = '\0';
		}

		/* Did we read a whole line? */
		if (buf[BUF_SIZE - 2] != '\0') {
			ts_error("%s: line %d too long\n", conffile, line);
			break;
		}

		tok = strsep(&p, " \t");
		
		/* Ignore comments or blank lines.
		 * Note: strsep modifies p (see man strsep)
		 */

		if (p==NULL || *tok == '#')
			continue;

		/* Search for the option. */
		if (strcasecmp(tok, "module") == 0) {
			module_name = strsep(&p, " \t");
			ret = ts_load_module(ts, module_name, p);
		}
		else if (strcasecmp(tok, "module_raw") == 0) {
			module_name = strsep(&p, " \t");
			ret = ts_load_module_raw(ts, module_name, p);
		} else {
			ts_error("%s: Unrecognised option %s:%d:%s\n", conffile, line, tok);
			break;
		}
		if (ret != 0) {
			ts_error("Couldnt load module %s\n", module_name);
			break;
		}
	}

	if (ts->list_raw == NULL) {
		ts_error("No raw modules loaded.\n");
		ret = -1;
	}

	fclose(f);

	return ret;
}
