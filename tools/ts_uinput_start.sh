#!/bin/bash

# This starts ts_uinput as a daemon and creates /dev/input/ts_uinput to use
# as an evdev input device

TS_UINPUT_SYSFS_NAME=$(ts_uinput -d)
if [[ "${TS_UINPUT_SYSFS_NAME}" != *"input"* ]] ; then
	echo "ts_uinput: failed to create uinput device"
	exit 1
fi

TS_UINPUT_SYSFS_PATH="/sys/devices/virtual/input/${TS_UINPUT_SYSFS_NAME}"
if [ ! -d "${TS_UINPUT_SYSFS_PATH}" ] ; then
	echo "ts_uinput: Error creating event device"
	exit 1
fi

TS_UINPUT_DEV_FILE=$(ls /sys/devices/virtual/input/$TS_UINPUT_SYSFS_NAME | grep '^event')
ln -s /dev/input/$TS_UINPUT_DEV_FILE /dev/input/ts_uinput
