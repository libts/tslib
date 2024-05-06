#ifndef _TSLIB_H_
#define _TSLIB_H_
/*
 *  tslib/src/tslib.h
 *
 *  Copyright (C) 2016 Martin Kepplinger <martink@posteo.de>
 *  Copyright (C) 2001 Russell King.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *
 * Touch screen library interface definitions.
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <stdarg.h>
#include <sys/time.h>

#ifdef WIN32
  #define TSIMPORT __declspec(dllimport)
  #define TSEXPORT __declspec(dllexport)
  #define TSLOCAL
#else
  #define TSIMPORT
  #ifdef GCC_HASCLASSVISIBILITY
    #define TSEXPORT __attribute__ ((visibility("default")))
    #define TSLOCAL __attribute__ ((visibility("hidden")))
  #else
    #define TSEXPORT
    #define TSLOCAL
  #endif
#endif

#ifdef TSLIB_INTERNAL
  #define TSAPI TSEXPORT
#else
  #define TSAPI TSIMPORT
#endif /* TSLIB_INTERNAL */

struct tsdev;

struct ts_sample {
	int		x;
	int		y;
	unsigned int	pressure;
	struct timeval	tv;
};

struct ts_sample_mt {
	/* ABS_MT_* event codes. linux/include/uapi/linux/input-event-codes.h
	 * has the definitions.
	 */
	int		x;
	int		y;
	unsigned int	pressure;
	int		slot;
	int		tracking_id;

	int		tool_type;
	int		tool_x;
	int		tool_y;
	unsigned int	touch_major;
	unsigned int	width_major;
	unsigned int	touch_minor;
	unsigned int	width_minor;
	int		orientation;
	int		distance;
	int		blob_id;

	struct timeval	tv;

	/* BTN_TOUCH state */
	short		pen_down;

	/* valid is set != 0 if this sample
	 * contains new data; see below for the
	 * bits that get set.
	 * valid is set to 0 otherwise
	 */
	short		valid;
};

#define TSLIB_MT_VALID			(1 << 0)	/* any new data */
#define TSLIB_MT_VALID_TOOL		(1 << 1)	/* new tool_x or tool_y data */

struct ts_lib_version_data {
	const char	*package_version;
	int		version_num;
	unsigned int	features; /* bitmask, see below */
};

#define TSLIB_VERSION_MT		(1 << 0)	/* multitouch support */
#define TSLIB_VERSION_OPEN_RESTRICTED	(1 << 1)	/* ts_open_restricted() */
#define TSLIB_VERSION_EVENTPATH		(1 << 2)	/* ts_get_eventpath() */
#define TSLIB_VERSION_VERSION		(1 << 3)	/* tslib_version() */

enum ts_param {
	TS_SCREEN_RES = 0,		/* 2 integer args, x and y */
	TS_SCREEN_ROT			/* 1 integer arg, 1 = rotate */
};

struct ts_module_conf {
	char *name;
	char *params;
	int raw;
	int nr;

	struct ts_module_conf *next;
	struct ts_module_conf *prev;
};

/*
 * Close the touchscreen device, free all resources.
 */
TSAPI int ts_close(struct tsdev *);

/*
 * Reloads all modules - useful to reload calibration data.
 */
TSAPI int ts_reconfig(struct tsdev *);

/*
 * Configure the touchscreen device.
 */
TSAPI int ts_config(struct tsdev *);

/*
 * Changes a setting.
 */
TSAPI int ts_option(struct tsdev *, enum ts_param, ...);

/*
 * Change this hook to point to your custom error handling function.
 */
extern TSAPI int (*ts_error_fn)(const char *fmt, va_list ap);

/*
 * Implement this to override open() for the input device and return the fd.
 */
extern TSAPI int (*ts_open_restricted)(const char *path, int flags, void *user_data);

/*
 * Implement this to override close().
 */
extern TSAPI void (*ts_close_restricted)(int fd, void *user_data);

/*
 * Returns the file descriptor in use for the touchscreen device.
 */
TSAPI int ts_fd(struct tsdev *);

/*
 * Load a filter/scaling module
 */
TSAPI int ts_load_module(struct tsdev *, const char *mod, const char *params);

/*
 * Open the touchscreen device.
 */
TSAPI struct tsdev *ts_open(const char *dev_name, int nonblock);

/*
 * Find, open and configure the touchscreen device.
 */
TSAPI struct tsdev *ts_setup(const char *dev_name, int nonblock);

/*
 * Return a scaled touchscreen sample.
 */
TSAPI int ts_read(struct tsdev *, struct ts_sample *, int);

/*
 * Returns a raw, unscaled sample from the touchscreen.
 */
TSAPI int ts_read_raw(struct tsdev *, struct ts_sample *, int);

/*
 * Return a scaled touchscreen multitouch sample.
 */
TSAPI int ts_read_mt(struct tsdev *, struct ts_sample_mt **, int slots, int nr);

/*
 * Return a raw, unscaled touchscreen multitouch sample.
 */
TSAPI int ts_read_raw_mt(struct tsdev *, struct ts_sample_mt **, int slots, int nr);

/*
 * This function returns a pointer to a static copy of the version info struct.
 */
TSAPI struct ts_lib_version_data *ts_libversion(void);

/*
 * Get the list of (commented-in) ts.conf module lines (as structs)
 */
TSAPI struct ts_module_conf *ts_conf_get(struct tsdev *ts);

/*
 * Write the list of modules to ts.conf
 */
TSAPI int ts_conf_set(struct ts_module_conf *conf);

/*
 * This function returns the path to the opened touchscreen input device file.
 */
TSAPI char *ts_get_eventpath(struct tsdev *tsdev);

/*
 * This simply returns tslib's version string
 */
TSAPI char *tslib_version(void);

/*
 * This prints tslib's logo to stdout, with pos preceding spaces
 */
TSAPI void ts_print_ascii_logo(unsigned int pos);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _TSLIB_H_ */
