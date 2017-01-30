#!/bin/bash
TS_UINPUT_SYSFS_NAME=$(ts_uinput -d)
if [ "$TS_UINPUT_SYSFS_NAME" == "" ] ; then
	exit 1
fi

if [ -d "/sys/devices/virtual/input/$TS_UINPUT_SYSFS_NAME" ] ; then
	TS_UINPUT_DEV_FILE=$(ls /sys/devices/virtual/input/$TS_UINPUT_SYSFS_NAME | grep '^event')
	ln -s /dev/input/$TS_UINPUT_DEV_FILE /dev/input/ts_uinput
	exit 0
else
	echo "ts_uinput: Error creating /dev/input/ts_uinput" >$2
	exit 1
fi
