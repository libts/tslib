#!/bin/bash

# This starts ts_uinput as a daemon and creates /dev/input/ts_uinput to use
# as an evdev input device

rm -f /dev/input/ts_uinput
TS_UINPUT_DEV_FILE=$(ts_uinput -d -v)

if [ -z "$TS_UINPUT_DEV_FILE" ]
then
	echo "ts_uinput: Error creating event device"
	exit 1
else
	ln -s /dev/input/$TS_UINPUT_DEV_FILE /dev/input/ts_uinput
fi

exit 0
