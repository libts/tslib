/*
 *  tslib/src/ts_config_filter.c
 *
 *  Copyright (C) 2018 Martin Kepplinger.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Interface for editing ts.conf. This edits "module" filters, not
 * "module_raw" access plugins.
 */
#include <stdlib.h>
#include <stdio.h>
#include "tslib-private.h"

#define MAX_LINES	200
#define LINE_MAX	1024

/* return a ordered list of structs. Each struct is one commented-in module */
struct ts_module_conf *ts_conf_get(struct tsdev *ts, const char *path)
{
	struct ts_module_conf *conf = NULL;
	struct ts_module_conf *conf_next = NULL;
	char **modulebuf = NULL;
	char **parambuf = NULL;
	int ret;
	int i;

	modulebuf = calloc(MAX_LINES, sizeof(char *));
	if (!modulebuf)
		goto fail;

	parambuf = calloc(MAX_LINES, sizeof(char *));
	if (!parambuf)
		goto fail;

	for (i = 0; i < MAX_LINES; i++) {
		modulebuf[i] = calloc(1, LINE_MAX);
		if (!modulebuf[i])
			goto fail;

		parambuf[i] = calloc(1, LINE_MAX);
		if (!parambuf[i])
			goto fail;
	}

	ret = ts_config_ro(ts, modulebuf, parambuf);
	if (ret)
		goto fail;

	for (i = 0; i < MAX_LINES; i++) {
		if (!modulebuf[i])
			continue;

		conf_next = calloc(1, sizeof(struct ts_module_conf));
		if (!conf_next)
			goto fail;

		conf_next->name = calloc(1, LINE_MAX);
		if (!conf_next->name)
			goto fail;

		sprintf(conf_next->name, modulebuf[i]);

		conf_next->params = calloc(1, LINE_MAX);
		if (!conf_next->params)
			goto fail;

		sprintf(conf_next->params, parambuf[i]);


		if (conf) {
			conf_next->prev = conf;
			conf->next = conf_next;
		}

		conf = conf_next;
		conf_next = NULL;
	}

	for (i = 0; i < MAX_LINES; i++) {
		free(modulebuf[i]);
		free(parambuf[i]);
	}
	free(modulebuf);
	free(parambuf);

	return conf;

fail:
	for (i = 0; i < MAX_LINES; i++) {
		free(modulebuf[i]);
		free(parambuf[i]);
	}
	free(modulebuf);
	free(parambuf);

	return NULL;
}

int ts_conf_set(struct tsdev *ts, const char *path, struct ts_module_conf *conf)
{
	return 0;
}
