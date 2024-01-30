#!/bin/sh
# SPDX-License-Identifier: GPL-2.0+
# Copyright (C) 2017, Martin Kepplinger <martink@posteo.de>

# This starts ts_uinput as a daemon and creates /dev/input/ts_uinput to use
# as an evdev input device

TS_UINPUT_DEV_FILE=$(ts_uinput -d -v | grep 'event')

if [ -n "$TS_UINPUT_DEV_FILE" ]
then
	rm -f /dev/input/ts_uinput
	ln -s "$TS_UINPUT_DEV_FILE" /dev/input/ts_uinput
else
	echo "ts_uinput: Error creating event device"
	exit 1
fi

exit 0
