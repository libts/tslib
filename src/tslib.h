#ifndef _TSLIB_H_
#define _TSLIB_H_
/*
 *  tslib/src/tslib.h
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.
 *
 * $Id: tslib.h,v 1.4 2005/02/26 01:47:23 kergoth Exp $
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
#endif // TSLIB_INTERNAL

struct tsdev;

struct ts_sample {
	int		x;
	int		y;
	unsigned int	pressure;
	struct timeval	tv;
};

/*
 * Close the touchscreen device, free all resources.
 */
TSAPI int ts_close(struct tsdev *);

/*
 * Configure the touchscreen device.
 */
TSAPI int ts_config(struct tsdev *);

/*
 * Change this hook to point to your custom error handling function.
 */
extern TSAPI int (*ts_error_fn)(const char *fmt, va_list ap);

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
 * Return a scaled touchscreen sample.
 */
TSAPI int ts_read(struct tsdev *, struct ts_sample *, int);

/*
 * Returns a raw, unscaled sample from the touchscreen.
 */
TSAPI int ts_read_raw(struct tsdev *, struct ts_sample *, int);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _TSLIB_H_ */
