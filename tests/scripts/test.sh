#!/bin/bash
# SPDX-License-Identifier: GPL-2.0+
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:
filtername=""
eventfile=""
verbose=0
descfile=""

function usage() {
	echo "Usage: ./evemu_test -f <filtername> -e \"<eventfile>\""
	if [ -d temp/evemu-devices ] ; then
		echo "available event files:"
		echo "----------------------"
		ls -1 temp/evemu-devices/touchscreens | grep events
	fi
}

while getopts "h?vf:e:" opt; do
    case "$opt" in
    h|\?)
	usage
        exit 0
        ;;
    v)  verbose=1
        ;;
    f)  filtername=$OPTARG
        ;;
    e)  eventfile=$OPTARG
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift


# trap ctrl-c and call ctrl_c()
trap ctrl_c INT

function ctrl_c() {
	if [ ! -f "${eventfile}.${filtername}.expected" ] ; then
		exit 1
	fi

	diff --report-identical-files \
		"${eventfile}.${filtername}.expected" \
		"result/${eventfile}.${filtername}.result"
}


if [ -z "$filtername" ] ; then
	echo -e "${YELLOW}Please provide the filter to test${NC}"
	usage
	exit 1
fi

mkdir -p temp
if [ ! -d temp/evemu-devices ] ; then
	git clone https://github.com/whot/evemu-devices.git temp/evemu-devices
fi

if [ -z "$eventfile" ] ; then
	echo -e "${YELLOW}Please provide the event file to use${NC}"
	ls -1 temp/evemu-devices/touchscreens | grep events
	usage
	exit 1
fi

if [ ! -f "${eventfile}.${filtername}.expected" ] ; then
	echo -e "${RED}WARNING: reference file doesn't yet exist${NC}"
fi

descfile=${eventfile%.*}
descfile=${descfile%.*}.desc

eventfile_path="temp/evemu-devices/touchscreens/${eventfile}"
descfile_path="temp/evemu-devices/touchscreens/${descfile}"

if [ ! -f "${eventfile_path}" ] ; then
	echo "given event file does not exist"
	usage
	exit 1
fi

if [ ! -f "${descfile_path}" ] ; then
	echo "no device descriptor file found"
	exit 1
fi

export TSLIB_CONFFILE=$(readlink -f ${filtername}.conf)
# TODO get the ts.conf on the commandline

if [ $verbose = 1 ] ; then
	echo -e "${YELLOW}ts.conf:${NC}"
	cat $TSLIB_CONFFILE
	echo ""
fi

TS_PRINT_MT=$(readlink -f ../ts_print_mt)

mkdir -p result

# create the virtual touchscreen via uinput
evemu-device "$descfile_path" | {
# this tries to catch the created device path
for runs in 1
do
  while IFS= read -r line
  do
    lastline="$line"
    for word in $lastline
    do
      :
    done

    # here we should have the device path
    if [ $verbose = 1 ] ; then
      echo -e "${YELLOW}$word${NC}"
    fi

    export TSLIB_TSDEVICE=$word

    if [ $verbose = 1 ] ; then
      echo -e "running ${GREEN}evemu-play $word < \"${eventfile_path}\"${NC}"
      echo "please verify the resulting samples you see and quit"
    fi

    $TS_PRINT_MT | grep sample | sed --expression='s/[[:digit:]]\+\.\+[[:digit:]]\+//g' > "result/${eventfile}.${filtername}.result" &

    # give tslib some time to start up
    sleep 1

    evemu-play $word < "${eventfile_path}"

  done
done
}
