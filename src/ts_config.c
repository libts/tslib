/*
 *  tslib/src/ts_config.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Read the configuration and load the appropriate drivers.
 */
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_LIBDL
#include <dlfcn.h>
#endif

#include <errno.h>

#if !defined(HAVE_STRSEP)
#include "ts_strsep.h"
#endif

#include "tslib-private.h"

/* Maximum line size is BUF_SIZE - 2
 * -1 for fgets and -1 to test end of line
 */
#define BUF_SIZE 512

/* Discard any empty tokens
 * Note: strsep modifies p (see man strsep)
 */
static void discard_null_tokens(char **p, char **tokPtr)
{
	while (*p != NULL && **tokPtr == '\0') {
	#if !defined HAVE_STRSEP
		*tokPtr = ts_strsep(p, "\t");
	#else
		*tokPtr = strsep(p, " \t");
	#endif
	}
}

static int __ts_config(struct tsdev *ts, char **conffile_modules,
		       char **conffile_params, int *raw)
{
	char buf[BUF_SIZE], *p;
	FILE *f;
	int line = 0;
	int ret = 0;
	short strdup_allocated = 0;

	char *conffile;

	if ((conffile = getenv("TSLIB_CONFFILE")) == NULL) {
		conffile = strdup(TS_CONF);
		if (conffile) {
			strdup_allocated = 1;
		} else {
			ts_error("Couldn't find tslib config file: %s\n",
				 strerror(errno));
			return -1;
		}
	}

	f = fopen(conffile, "r");
	if (!f) {
		if (strdup_allocated)
			free(conffile);

		ts_error("Couldn't open tslib config file %s: %s\n",
			 conffile, strerror(errno));
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
		if (e)
			*e = '\0';

		/* Did we read a whole line? */
		if (buf[BUF_SIZE - 2] != '\0') {
			ts_error("%s: line %d too long\n", conffile, line);
			break;
		}

	#if !defined HAVE_STRSEP
		tok = ts_strsep(&p, " \t");
	#else
		tok = strsep(&p, " \t");
	#endif
		discard_null_tokens(&p, &tok);

		/* Ignore comments or blank lines.
		 * Note: strsep modifies p (see man strsep)
		 */
		if (p == NULL || *tok == '#')
			continue;

		/* Search for the option. */
		if (strcasecmp(tok, "module") == 0) {
		#if !defined HAVE_STRSEP
			module_name = ts_strsep(&p, " \t");
		#else
			module_name = strsep(&p, " \t");
		#endif
			discard_null_tokens(&p, &module_name);
			if (!conffile_modules) {
				ret = ts_load_module(ts, module_name, p);
			} else {
			#ifdef DEBUG
				printf("TSLIB_CONFFILE: module %s %s\n",
					module_name, p);
			#endif
				sprintf(conffile_modules[line], "%s", module_name);
				if (conffile_params && p)
					sprintf(conffile_params[line], "%s", p);
			}
		} else if (strcasecmp(tok, "module_raw") == 0) {
		#if !defined HAVE_STRSEP
			module_name = ts_strsep(&p, " \t");
		#else
			module_name = strsep(&p, " \t");
		#endif
			discard_null_tokens(&p, &module_name);
			if (!conffile_modules) {
				ret = ts_load_module_raw(ts, module_name, p);
			} else {
			#ifdef DEBUG
				printf("TSLIB_CONFFILE: module_raw %s %s\n",
					module_name, p);
			#endif
				sprintf(conffile_modules[line], "%s", module_name);
				if (conffile_params && p)
					sprintf(conffile_params[line], "%s", p);

				if (raw)
					raw[line] = 1;
			}
		} else {
			ts_error("%s: Unrecognised option %s:%d:%s\n",
				 conffile, line, tok);
			break;
		}
		if (ret != 0) {
			ts_error("Couldn't load module %s\n", module_name);
			break;
		}
	}

	if (ts->list_raw == NULL) {
		ts_error("No raw modules loaded.\n");
		ret = -1;
	}

	fclose(f);

	if (strdup_allocated)
		free(conffile);

	return ret;
}

int ts_config_ro(struct tsdev *ts, char **conffile_modules,
		 char **conffile_params, int *raw)
{
	return __ts_config(ts, conffile_modules, conffile_params, raw);
}

int ts_config(struct tsdev *ts)
{
	return __ts_config(ts, NULL, NULL, NULL);
}

int ts_reconfig(struct tsdev *ts)
{
	void *handle;
	int ret;
	struct tslib_module_info *info, *next;
	int fd;

	info = ts->list;
	while (info) {
		/* Save the "next" pointer now because info will be freed */
		next = info->next;

		handle = info->handle;

		if (info->ops->fini)
			info->ops->fini(info);
	#ifdef HAVE_LIBDL
		if (handle)
			dlclose(handle);
	#endif

		info = next;
	}

	fd = ts->fd;	/* save temp */
	memset(ts, 0, sizeof(struct tsdev));
	ts->fd = fd;

	ret = ts_config(ts);
	return ret;
}
