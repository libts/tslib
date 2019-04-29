/*
 *  tslib/src/ts_setup.c
 *
 *  Copyright (C) 2017 Piotr Figlarek
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 *
 *
 * Find, open and configure a touchscreen device.
 */

#include "tslib.h"
#include "tslib-private.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#if defined (__linux__)
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

/* for old kernel headers */
#ifndef INPUT_PROP_MAX
# define INPUT_PROP_MAX			0x1f
#endif
#ifndef INPUT_PROP_DIRECT
# define INPUT_PROP_DIRECT		0x01
#endif

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define BIT(nr)                 (1UL << (nr))
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BITS_PER_BYTE           8
#define BITS_PER_LONG           (sizeof(long) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

static int is_event_device(const struct dirent *dir)
{
	return strncmp(EVENT_DEV_NAME, dir->d_name, 5) == 0;
}

static char *scan_devices(void)
{
	struct dirent **namelist;
	int i, ndev;
	char *filename = NULL;
	long propbit[BITS_TO_LONGS(INPUT_PROP_MAX)] = {0};

#ifdef DEBUG
	printf("scanning for devices in %s\n", DEV_INPUT_EVENT);
#endif

	ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, alphasort);
	if (ndev <= 0)
		return NULL;

	for (i = 0; i < ndev; i++) {
		char fname[512];
		int fd = -1;

		snprintf(fname, sizeof(fname),
			 "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);
		fd = open(fname, O_RDONLY);
		if (fd < 0)
			continue;

		if ((ioctl(fd, EVIOCGPROP(sizeof(propbit)), propbit) < 0) ||
			!(propbit[BIT_WORD(INPUT_PROP_DIRECT)] &
				  BIT_MASK(INPUT_PROP_DIRECT))) {
			close(fd);
			continue;
		} else {
			close(fd);
			filename = malloc(strlen(DEV_INPUT_EVENT) +
					  strlen(EVENT_DEV_NAME) +
					  12);
			if (!filename)
				break;

			sprintf(filename, "%s/%s%d",
				DEV_INPUT_EVENT, EVENT_DEV_NAME,
				i);
			break;
		}
	}

	for (i = 0; i < ndev; ++i)
		free(namelist[i]);

	free(namelist);

	return filename;
}

#endif /* __linux__ */

static const char * const ts_name_default[] = {
		"/dev/input/ts",
		"/dev/input/touchscreen",
		"/dev/touchscreen/ucb1x00",
		NULL
};

struct tsdev *ts_setup(const char *dev_name, int nonblock)
{
	const char * const *defname;
	struct tsdev *ts = NULL;
#if defined (__linux__)
	char *fname = NULL;
#endif /* __linux__ */

	dev_name = dev_name ? dev_name : getenv("TSLIB_TSDEVICE");

	if (dev_name != NULL) {
		ts = ts_open(dev_name, nonblock);
	} else {
		defname = &ts_name_default[0];
		while (*defname != NULL) {
			ts = ts_open(*defname, nonblock);
			if (ts != NULL)
				break;

			++defname;
		}
	}

#if defined (__linux__)
	if (!ts) {
		fname = scan_devices();
		if (!fname)
			return NULL;

		ts = ts_open(fname, nonblock);
		free(fname);
	}
#endif /* __linux__ */

	/* if detected try to configure it */
	if (ts && ts_config(ts) != 0) {
		ts_error("ts_config: %s\n", strerror(errno));
		ts_close(ts);
		return NULL;
	}

	return ts;
}
