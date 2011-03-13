/*
 * tslib driver for DMC touchscreens
 * Copyright (C) 2009 Michael Olbrich, Pengutronix e.K.
 * some inspiration from the old xf86-input-dmc xorg input driver.
 *
 * This file is placed under the LGPL.  Please see the file COPYING for more
 * details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <termios.h>

#include "config.h"
#include "tslib-private.h"

struct tslib_dmc {
	struct tslib_module_info module;

	int	current_x;
	int	current_y;
	int	sane_fd;
};

int dmc_init_device(struct tsdev *dev)
{
	int fd = dev->fd;
	struct termios t;
	char buf[1];

	/* flags from the old xorg driver.
	 * I have no idea what all of these mean but it works (tm). */
	tcgetattr(fd, &t);
	t.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON|IXOFF);

	t.c_oflag &= ~OPOST;

	t.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);

	t.c_cflag &= ~(CSIZE|PARENB|CSTOPB);
	t.c_cflag |= CS8|CLOCAL;

	cfsetispeed (&t, B9600);
	cfsetospeed (&t, B9600);

	t.c_cc[VMIN] = 3;
	t.c_cc[VTIME] = 1;

	tcsetattr(fd, TCSANOW, &t);

	if (write(fd, "\x55", 1) != 1) {
		fprintf(stderr, "dmc: failed to write. Check permissions of the device!\n");
		return EINVAL;
	}
	sleep(1);
	if (write(fd, "\x05\x40", 2) != 2) {
		perror("dmc write");
		goto fail;
	}
	if (read(fd, buf, 1) != 1) {
		perror("dmc read");
		goto fail;
	}
	if (buf[0] != 0x6) {
		fprintf(stderr, "dmc: got wrong return value. The touchscreen may not work.\n");
	}
	if (write(fd, "\x31", 1) != 1) {
		perror("dmc write");
		goto fail;
	}
	return 0;
fail:
	fprintf(stderr, "dmc: selected device is not a touchscreen I understand\n");
	return EINVAL;
}

static int dmc_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tslib_dmc *dmc = (struct tslib_dmc*)inf;
	struct tsdev *ts = inf->dev;
	uint8_t buf[5];
	int ret;
	int i;

	for (i = 0; i < nr; ++i) {
		if ((ret = read(ts->fd, buf, 1)) != 1) {
			--i;
			break;
		}
		if (buf[0] == 0x10) {
			/* release. No coords follow. Use old values */
			samp->x = dmc->current_x;
			samp->y = dmc->current_y;
			samp->pressure = 0;
		}
		else if (buf[0] == 0x11) {
			/* read coords */
			if ((ret = read(ts->fd, buf, 4)) != 4) {
				/* must have 4 bytes */
				--i;
				break;
			}
			samp->x = dmc->current_x = (int)((buf[0] << 8) + buf[1]);
			samp->y = dmc->current_y = (int)((buf[2] << 8) + buf[3]);
			samp->pressure = 100;
		}
		else
			continue;
#ifdef DEBUG
		fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
		gettimeofday(&samp->tv,NULL);
		++samp;
	}
	return i;
}

static const struct tslib_ops dmc_ops =
{
	.read	= dmc_read,
};

TSAPI struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_dmc *m;

	if (dmc_init_device(dev) != 0)
		return 0;

	m = calloc(1, sizeof(struct tslib_dmc));
	if (m == NULL)
		return NULL;

	m->module.ops = &dmc_ops;
	return (struct tslib_module_info*)m;
}
