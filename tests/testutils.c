/*
 *  tslib/src/ts_getxy.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Waits for the screen to be touched, averages x and y sample
 * coordinates until the end of contact
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "tslib.h"
#include "fbutils.h"
#include "testutils.h"

/* [inactive] border fill text [active] border fill text */
int button_palette [6] =
{
        1, 4, 2,
        1, 5, 0
};

void button_draw (struct ts_button *button)
{
	int s = (button->flags & BUTTON_ACTIVE) ? 3 : 0;

	rect(button->x, button->y, button->x + button->w - 1,
	     button->y + button->h - 1, button_palette [s]);
	fillrect(button->x + 1, button->y + 1,
		 button->x + button->w - 2,
		 button->y + button->h - 2, button_palette [s + 1]);
	put_string_center(button->x + button->w / 2,
			  button->y + button->h / 2,
			  button->text, button_palette [s + 2]);
}

int button_handle (struct ts_button *button, int x, int y, unsigned int p)
{
	int inside = (x >= button->x) && (y >= button->y) &&
		     (x < button->x + button->w) &&
		     (y < button->y + button->h);

	if (p > 0) {
		if (inside) {
			if (!(button->flags & BUTTON_ACTIVE)) {
				button->flags |= BUTTON_ACTIVE;
				button_draw (button);
			}
		} else if (button->flags & BUTTON_ACTIVE) {
			button->flags &= ~BUTTON_ACTIVE;
			button_draw (button);
		}
	} else if (button->flags & BUTTON_ACTIVE) {
		button->flags &= ~BUTTON_ACTIVE;
		button_draw (button);
		return 1;
	}

	return 0;
}

static int sort_by_x(const void* a, const void *b)
{
	return (((struct ts_sample *)a)->x - ((struct ts_sample *)b)->x);
}

static int sort_by_y(const void* a, const void *b)
{
	return (((struct ts_sample *)a)->y - ((struct ts_sample *)b)->y);
}

void getxy(struct tsdev *ts, int *x, int *y)
{
#define MAX_SAMPLES 128
	struct ts_sample samp[MAX_SAMPLES];
	int index, middle;

	do {
		if (ts_read_raw(ts, &samp[0], 1) < 0) {
			perror("ts_read");
			close_framebuffer ();
			exit(1);
		}
		
	} while (samp[0].pressure == 0);

	/* Now collect up to MAX_SAMPLES touches into the samp array. */
	index = 0;
	do {
		if (index < MAX_SAMPLES-1)
			index++;
		if (ts_read_raw(ts, &samp[index], 1) < 0) {
			perror("ts_read");
			close_framebuffer ();
			exit(1);
		}
	} while (samp[index].pressure > 0);
	printf("Took %d samples...\n",index);

	/*
	 * At this point, we have samples in indices zero to (index-1)
	 * which means that we have (index) number of samples.  We want
	 * to calculate the median of the samples so that wild outliers
	 * don't skew the result.  First off, let's assume that arrays
	 * are one-based instead of zero-based.  If this were the case
	 * and index was odd, we would need sample number ((index+1)/2)
	 * of a sorted array; if index was even, we would need the
	 * average of sample number (index/2) and sample number
	 * ((index/2)+1).  To turn this into something useful for the
	 * real world, we just need to subtract one off of the sample
	 * numbers.  So for when index is odd, we need sample number
	 * (((index+1)/2)-1).  Due to integer division truncation, we
	 * can simplify this to just (index/2).  When index is even, we
	 * need the average of sample number ((index/2)-1) and sample
	 * number (index/2).  Calculate (index/2) now and we'll handle
	 * the even odd stuff after we sort.
	 */
	middle = index/2;
	if (x) {
		qsort(samp, index, sizeof(struct ts_sample), sort_by_x);
		if (index & 1)
			*x = samp[middle].x;
		else
			*x = (samp[middle-1].x + samp[middle].x) / 2;
	}
	if (y) {
		qsort(samp, index, sizeof(struct ts_sample), sort_by_y);
		if (index & 1)
			*y = samp[middle].y;
		else
			*y = (samp[middle-1].y + samp[middle].y) / 2;
	}
}

void ts_flush (struct tsdev *ts)
{
	/* Read all unread touchscreen data, 
	 * so that we are sure that the next data that we read
	 * have been input after this flushing.
	 */

#define TS_BUFFER_MAX 32768
	static char buffer [TS_BUFFER_MAX];
	if (read (ts_fd (ts), buffer, TS_BUFFER_MAX) == -1)
		fprintf(stderr, "ts_flush read error\n");
}
