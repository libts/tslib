/*
 *  tslib/src/ts_load_module.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Share and static plugin-modules loading functions
 */
#include "config.h"

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBDL
#include <dlfcn.h>
#endif

#include "tslib-private.h"

#include "../plugins/plugins.h"

struct tslib_module_desc {
	const char *name;
	tslib_module_init mod_init;
};

static const struct tslib_module_desc tslib_modules[] = {
	/* sort alphabetically */
#ifdef TSLIB_STATIC_ARCTIC2_MODULE
	{ "arctic2", arctic2_mod_init },
#endif
#ifdef TSLIB_STATIC_COLLIE_MODULE
	{ "collie", collie_mod_init },
#endif
#ifdef TSLIB_STATIC_CORGI_MODULE
	{ "corgi", corgi_mod_init },
#endif
#ifdef TSLIB_STATIC_CY8MRLN_PALMPRE_MODULE
	{ "cy8mrln_palmpre", cy8mrln_palmpre_mod_init },
#endif
#ifdef TSLIB_STATIC_DEBOUNCE_MODULE
	{ "debounce", debounce_mod_init },
#endif
#ifdef TSLIB_STATIC_DEJITTER_MODULE
	{ "dejitter", dejitter_mod_init },
#endif
#ifdef TSLIB_STATIC_DMC_MODULE
	{ "dmc", dmc_mod_init },
#endif
#ifdef TSLIB_STATIC_DMC_DUS3000_MODULE
	{ "dmc_dus3000", dmc_dus3000_mod_init },
#endif
#ifdef TSLIB_STATIC_EVTHRES_MODULE
	{ "evthres", evthres_mod_init },
#endif
#ifdef TSLIB_STATIC_GALAX_MODULE
	{ "galax", galax_mod_init },
#endif
#ifdef TSLIB_STATIC_H3600_MODULE
	{ "h3600", h3600_mod_init },
#endif
#ifdef TSLIB_STATIC_IIR_MODULE
	{ "iir", iir_mod_init },
#endif
#ifdef TSLIB_STATIC_INPUT_EVDEV_MODULE
	{ "input_evdev", input_evdev_mod_init },
#endif
#ifdef TSLIB_STATIC_INPUT_MODULE
	{ "input", input_mod_init },
#endif
#ifdef TSLIB_STATIC_INVERT_MODULE
	{ "invert", invert_mod_init },
#endif
#ifdef TSLIB_STATIC_LINEAR_MODULE
	{ "linear", linear_mod_init },
#endif
#ifdef TSLIB_STATIC_LINEAR_H2200_MODULE
	{ "linear_h2200", linear_h2200_mod_init },
#endif
#ifdef TSLIB_STATIC_LOWPASS_MODULE
	{ "lowpass", lowpass_mod_init },
#endif
#ifdef TSLIB_STATIC_MEDIAN_MODULE
	{ "median", median_mod_init },
#endif
#ifdef TSLIB_STATIC_MK712_MODULE
	{ "mk712", mk712_mod_init },
#endif
#ifdef TSLIB_STATIC_ONE_WIRE_TS_INPUT_MODULE
	{ "one_wire_ts_input", one_wire_ts_input_mod_init },
#endif
#ifdef TSLIB_STATIC_PTHRES_MODULE
	{ "pthres", pthres_mod_init },
#endif
#ifdef TSLIB_STATIC_SKIP_MODULE
	{ "skip", skip_mod_init },
#endif
#ifdef TSLIB_STATIC_TATUNG_MODULE
	{ "tatung", tatung_mod_init },
#endif
#ifdef TSLIB_STATIC_TOUCHKIT_MODULE
	{ "touchkit", touchkit_mod_init },
#endif
#ifdef TSLIB_STATIC_UCB1X00_MODULE
	{ "ucb1x00", ucb1x00_mod_init },
#endif
#ifdef TSLIB_STATIC_VARIANCE_MODULE
	{ "variance", variance_mod_init },
#endif
#ifdef TSLIB_STATIC_WAVESHARE_MODULE
	{ "waveshare", waveshare_mod_init },
#endif
};

#define countof(arr) (sizeof(arr) / sizeof((arr)[0]))

static int cmp_name(const void *va, const void *vb)
{
	struct tslib_module_desc *a = (struct tslib_module_desc *)va;
	struct tslib_module_desc *b = (struct tslib_module_desc *)vb;

	return strcmp(a->name, b->name);
}

static struct tslib_module_info *__ts_load_module_static(struct tsdev *ts,
							 const char *module,
							 const char *params)
{
	struct tslib_module_info *info = NULL;
	struct tslib_module_desc *result;
	struct tslib_module_desc key;

	key.name = module;
	result = bsearch(&key, tslib_modules, countof(tslib_modules),
			 sizeof(struct tslib_module_desc), cmp_name);
	if (!result)
		return NULL;

	info = result->mod_init(ts, params);
#ifdef DEBUG
	fprintf(stderr, "static module %s init %s\n", module,
		info ? "succeeded" : "failed");
#endif

	if (info)
		info->handle = NULL;

	return info;
}

#define PLUGIN_DIR_LEN 1024

#ifdef HAVE_LIBDL
static struct tslib_module_info *__ts_load_module_shared(struct tsdev *ts,
							 const char *module,
							 const char *params)
{
	tslib_module_init *init;
	struct tslib_module_info *info;
	char fn[PLUGIN_DIR_LEN];
	void *handle;
	char *plugin_directory = getenv("TSLIB_PLUGINDIR");

	if (!plugin_directory)
		plugin_directory = PLUGIN_DIR;

	if (strlen(plugin_directory) >= PLUGIN_DIR_LEN) {
#ifdef DEBUG
		ts_error("%s dlopen() failed: path too long\n", fn);
#endif
		return NULL;
	}

	snprintf(fn, sizeof(fn), "%s/%s.so", plugin_directory, module);

	handle = dlopen(fn, RTLD_NOW);
	if (!handle) {
#ifdef DEBUG
		ts_error("%s dlopen() failed: %s\n", fn, dlerror());
#endif
		return NULL;
	}

	init = dlsym(handle, "mod_init");
	if (!init || !(*init)) {
#ifdef DEBUG
		ts_error("%s dlsym() failed: %s\n", fn, dlerror());
#endif
		dlclose(handle);
		return NULL;
	}

	info = (*init)(ts, params);
	if (!info) {
#ifdef DEBUG
		ts_error("Can't init %s\n", fn);
#endif
		dlclose(handle);
		return NULL;
	}

	info->handle = handle;

	return info;
}
#endif /* HAVE_LIBDL */

static int __ts_load_module(struct tsdev *ts, const char *module,
			    const char *params, int raw)
{
	struct tslib_module_info *info;
	void *handle;
	int ret;

#ifdef DEBUG
	if (params)
		printf("Loading module %s (%s)\n", module, params);
	else
		printf("Loading module %s\n", module);
#endif

	info = __ts_load_module_static(ts, module, params);
#ifdef HAVE_LIBDL
	if (!info)
		info = __ts_load_module_shared(ts, module, params);
#endif
	if (!info)
		return -1;

	if (raw)
		ret = __ts_attach_raw(ts, info);
	else
		ret = __ts_attach(ts, info);

	if (ret) {
#ifdef DEBUG
		ts_error("Can't attach %s\n", module);
#endif
		handle = info->handle;

		if (info->ops->fini)
			info->ops->fini(info);

#ifdef HAVE_LIBDL
		if (handle)
			dlclose(handle);
#endif
	}

	return ret;
}

int ts_load_module(struct tsdev *ts, const char *module, const char *params)
{
	return __ts_load_module(ts, module, params, 0);
}

int ts_load_module_raw(struct tsdev *ts, const char *module, const char *params)
{
	return __ts_load_module(ts, module, params, 1);
}
