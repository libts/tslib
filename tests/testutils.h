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

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define YELLOW  "\033[33m"

struct ts_button {
	int x, y, w, h;
	char *text;
	int flags;
#define BUTTON_ACTIVE 0x00000001
};

void button_draw(struct ts_button *button);
int button_handle(struct ts_button *button, int x, int y, unsigned int pressure);
void getxy(struct tsdev *ts, int *x, int *y);
void ts_flush (struct tsdev *ts);

#endif /* _TESTUTILS_H */
