#!/bin/bash

# This starts ts_uinput as a daemon and creates /dev/input/ts_uinput to use
# as an evdev input device

TS_UINPUT_DEV_FILE=$(ts_uinput -d -v)
TS_UINPUT_DEV_FILE_CHECKED=$(ls "$TS_UINPUT_DEV_FILE" | grep 'event')

if [ ! -z "$TS_UINPUT_DEV_FILE_CHECKED" ]
then
	rm -f /dev/input/ts_uinput
	ln -s $TS_UINPUT_DEV_FILE_CHECKED /dev/input/ts_uinput
else
	echo "ts_uinput: Error creating event device"
	exit 1
fi

exit 0
