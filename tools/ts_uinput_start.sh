#!/bin/bash

# This starts ts_uinput as a daemon and creates /dev/input/ts_uinput to use
# as an evdev input device

TS_UINPUT_SYSFS_NAME=$(ts_uinput -d)
TS_UINPUT_CHECK="input"
if [ "${TS_UINPUT_SYSFS_NAME}/$TS_UINPUT_CHECK" = "${TS_UINPUT_SYSFS_NAME}" ]
then
        echo "ts_uinput: failed to create uinput device ${TS_UINPUT_SYSFS_NAME}"
        exit 1
fi
TS_UINPUT_SYSFS_PATH="/sys/devices/virtual/input/${TS_UINPUT_SYSFS_NAME}"
if [ ! -d "${TS_UINPUT_SYSFS_PATH}" ] ; then
        echo "ts_uinput: Error creating event device"
        exit 1
fi

rm -f /dev/input/ts_uinput
TS_UINPUT_DEV_FILE=$(ls /sys/devices/virtual/input/$TS_UINPUT_SYSFS_NAME | grep '^event')
ln -s /dev/input/$TS_UINPUT_DEV_FILE /dev/input/ts_uinput
exit 0
