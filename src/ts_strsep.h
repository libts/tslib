/*
 *  tslib/src/ts_strsep.h
 *
 * Copyright (C) 2017 Martin Kepplinger <martink@posteo.de>
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1
 */
#include <stddef.h>
#include <string.h>
#include <stdio.h>

char *ts_strsep(char **stringp, const char *delim);
