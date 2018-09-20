/*
 * Copyright 2017 Andreas Hartmetz <ahartmetz@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

/*
 * DMC projected capacitive touch screen - DUS Series (tested with DUS3000)
 *
 * - UART and winxp mode only
 * - module linear and running ts_calibrate once is required
 *   (on-board calibration is not triggered)
 *
 * From the Software Spec, Copyright 2014 DMC Co., Ltd.:
 *
 * UART Interface
 * ---------------------
 * Baud rate	57600
 * Data bit	8bits
 * Parity	None
 * Stop bit	1bit
 * Flow control	None
 *
 * Transmission of the coordinate data will start once the host computer sends
 * instruction for starting via coordinate output control command.
 *
 * Report ID is 0x01, and mouse format (SW: 0x00=up, 0x01=down) will be placed after it.
 * 0		1		2		3		4		5
 * Report ID	SW		X (lower byte)	X (upper)	Y (lower)	Y (upper)
 *
 *
 * TODO There is a USB and multitouch mode too. Support that in Linux' usbtouchscreen.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <termios.h>

#include "config.h"
#include "tslib-private.h"

typedef enum {
	tr_id_none       = 0x00,
	tr_id_cal_offset = 0x01,
	tr_id_vers_info  = 0x04,
	tr_id_firmw_info = 0x06,
	tr_id_adj_offset = 0x17
} TTouchResponseIDs;

typedef enum {
	tm_version_info,
	tm_detailed_firmw_info,
	tm_setup_winxp,
	tm_coordinates_on,
	tm_coordinates_off,
	tm_adjust_offset,
	tm_calibrate_offset,
	tm_NumberOfTouchMessages
} TTouchMessages;

typedef enum {
	AdjustOffset = 0,
	CalibrateOffset,
	Coordinates
} TState;

struct tslib_dus3000 {
	struct tslib_module_info module;

	TState state;
	ssize_t rxPosition;
	uint8_t rxBuffer[0x110]; // messages with length field have 3 bytes + (contents of length field)
				 // bytes (so max 255) + one null byte we add for strings; round that up
};

static void dus3000_init_data(struct tslib_dus3000 *d)
{
	d->state = Coordinates;
	d->rxPosition = 0;
}

struct Command {
	uint8_t length;
	uint8_t data[5];
};

static int send_command(int fd, TTouchMessages id)
{
	static const struct Command commands[tm_NumberOfTouchMessages] = {
		{ 5, { 0x02, 0x4C, 0x02, 0x04, 0x00 } }, // tm_version_info
		{ 5, { 0x02, 0x4C, 0x02, 0x06, 0x00 } }, // tm_detailed_firmw_info
		{ 5, { 0x02, 0x4C, 0x02, 0x80, 0x04 } }, // tm_setup_winxp
		{ 5, { 0x02, 0x4C, 0x02, 0x81, 0x01 } }, // tm_coordinates_on
		{ 5, { 0x02, 0x4C, 0x02, 0x81, 0x00 } }, // tm_coordinates_off
		{ 4, { 0x02, 0x4C, 0x01, 0x17, 0x00 } }, // tm_adjust_offset
		{ 4, { 0x02, 0x4C, 0x01, 0x01, 0x00 } }  // tm_calibrate_offset
	};
	const ssize_t length = commands[id].length;
	// writing synchronously to an fd opened in async mode is slightly dubious, but
	// we can reasonably expect that such small amounts of data are buffered on the
	// sending (our) side.
	return write(fd, commands[id].data, length) == length;
}

static void dus3000_init_device(struct tslib_dus3000 *d, struct tsdev *dev)
{
	const int fd = dev->fd;
	struct termios tty;

	tcgetattr(fd, &tty);
	tty.c_iflag = IGNBRK | IGNPAR;
	tty.c_oflag = 0;
	tty.c_lflag = 0;
	tty.c_line = 0;
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 1;
	tty.c_cflag = CS8 | CREAD | CLOCAL | HUPCL;
	cfsetispeed(&tty, B57600);
	cfsetospeed(&tty, B57600);
	tcsetattr(fd, TCSAFLUSH, &tty);

	// Using XP mode because that's the easiest way to get single-touch without filtering
	// which finger(s) to accept.
	send_command(fd, tm_setup_winxp);

	switch (d->state) {
	case AdjustOffset:
		send_command(fd, tm_adjust_offset);
		break;
	case CalibrateOffset:
		send_command(fd, tm_calibrate_offset);
		break;
	case Coordinates:
		send_command(fd, tm_coordinates_on);
		break;
	}
}

static int read_to_pos(int fd, struct tslib_dus3000 *d, ssize_t pos)
{
	ssize_t toRead = pos - d->rxPosition;

	if (toRead <= 0)
		return 1;

	ssize_t r = read(fd, &d->rxBuffer[d->rxPosition], toRead);

	if (r >= 0) {
		d->rxPosition += r;
		return 1;
	} else {
		return 0;
	}
}

static int dus3000_read(struct tslib_module_info *m, struct ts_sample *samples, int maxSamples)
{
	struct tslib_dus3000 *const d = (struct tslib_dus3000 *)m;
	const int fd = m->dev->fd;
	int i = 0;

	for (i = 0; i < maxSamples; ) {
		// - Read until we have at least four bytes, which is <= minimum reply length and also
		//   sufficient to determine the final length of all kinds of replies
		// then... code:
		if (!read_to_pos(fd, d, 4))
			break;

		if (d->rxPosition >= 4) {
			if (d->rxBuffer[0] == 0x01) {
				if (!read_to_pos(fd, d, 6))
					break;

				// read finger up / down and coordinates
				samples[i].pressure = d->rxBuffer[1] ? 255 : 0;
				samples[i].x = ((int)d->rxBuffer[2]) | (((int)d->rxBuffer[3]) << 8);
				samples[i].y = ((int)d->rxBuffer[4]) | (((int)d->rxBuffer[5]) << 8);
				i++;
				d->rxPosition = 0; // message complete

			} else if (d->rxBuffer[0] == 0x02 && d->rxBuffer[1] == 0x4c) {
				const ssize_t fullRequiredLength = 3 + d->rxBuffer[2];

				if (!read_to_pos(fd, d, fullRequiredLength))
					break;

				d->rxPosition = 0; // message complete

				switch (d->rxBuffer[3]) {
				case tr_id_vers_info: // "acquisition of version information" response
					// get rest of message. length is stored in m_RxBuf[2]
#ifdef DEBUG
					d->rxBuffer[fullRequiredLength] = '\0';
					fprintf(stderr, "Version information: %s\n", d->rxBuffer + 3);
#endif
					break;

				case tr_id_firmw_info: // "firmware detailed information" response
#ifdef DEBUG
					d->rxBuffer[fullRequiredLength] = '\0';
					fprintf(stderr, "Firmware information: %s\n", d->rxBuffer);
#endif
					break;

				case tr_id_adj_offset: // "adjust offset" response
					if (d->state == AdjustOffset && d->rxBuffer[4] != 0) {
#ifdef DEBUG
						fprintf(stderr, "Adjust offset succeeded!\n");
#endif
					} else {
						// abort calibration if command transmission failed
						continue;
					}
					if (!send_command(fd, tm_calibrate_offset)) {
						fprintf(stderr,
							"Calibrate offset command transmission failed!\n");
						continue;
					}
					d->state = CalibrateOffset;
					break;

				case tr_id_cal_offset: // "calibrate offset" response
					if (d->state == CalibrateOffset && d->rxBuffer[4] != 0) {
#ifdef DEBUG
						fprintf(stderr, "Calibrate offset succeeded!\n");
#endif
					} else {
						// abort calibration if command transmission failed
						fprintf(stderr, "Calibrate offset failed!\n");
						continue;
					}
					// calibration done, restart coordinate transmission
					if (!send_command(fd, tm_coordinates_on)) {
						fprintf(stderr,
							"Enable coordinates command transmission failed!\n");
						continue;
					}
					d->state = Coordinates;
#ifdef DEBUG
					fprintf(stderr, "Calibration finished!\n");
#endif
					break;

				} // switch
			} else {
				fprintf(stderr,
					"DUS3000: unknown response 0x %02x %02x %02x %02x - resynchronizing!\n",
					d->rxBuffer[0], d->rxBuffer[1], d->rxBuffer[2], d->rxBuffer[3]);

				// stop coordinate transmission, flush buffers, and re-enable
				send_command(fd, tm_coordinates_off);
				usleep (50000); // let all pending data arrive in our buffer
				tcflush(fd, TCIOFLUSH); // clear buffers
				d->rxPosition = 0;
				d->state = Coordinates;
				send_command(fd, tm_coordinates_on);

			}
		}
	}

	return i;
}

static int dus3000_fini(struct tslib_module_info *inf)
{
	free(inf);

	return 0;
}

static const struct tslib_ops dus3000_ops = {
	.read	= dus3000_read,
	.fini	= dus3000_fini,
};

TSAPI struct tslib_module_info *dmc_dus3000_mod_init(struct tsdev *dev,
			    __attribute__ ((unused)) const char *params)
{
	struct tslib_dus3000 *d = calloc(1, sizeof(struct tslib_dus3000));

	if (d == NULL)
		return NULL;

	dus3000_init_data(d);
	dus3000_init_device(d, dev);

	d->module.ops = &dus3000_ops;
	return &(d->module);
}

#ifndef TSLIB_STATIC_DMC_DUS3000_MODULE
	TSLIB_MODULE_INIT(dmc_dus3000_mod_init);
#endif
