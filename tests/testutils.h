#ifndef _TESTUTILS_H
#define _TESTUTILS_H
/*
 *  tslib/tests/testutils.h
 *
 *  Copyright (C) 2004 Michael Opdenacker <michaelo@handhelds.org>
 *
 * This file is placed under the LGPL.
 *
 *
 * Misc utils for ts test programs
 */

void getxy(struct tsdev *ts, int *x, int *y);
void ts_flush (struct tsdev *ts);

#endif /* _TESTUTILS_H */
