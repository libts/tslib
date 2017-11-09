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
eventfile=""
verbose=0
descfile=""

function usage() {
	echo "Usage: ./evemu_test -e \"<eventfile>\""
}

while getopts "h?ve:" opt; do
    case "$opt" in
    h|\?)
	usage
        exit 0
        ;;
    v)  verbose=1
        ;;
    e)  eventfile=$OPTARG
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

if [ -z "$eventfile" ] ; then
	echo -e "${YELLOW}Please provide the event file to use${NC}"
	usage
	exit 1
fi

# descfile=${eventfile%.*}
# descfile=${descfile%.*}.desc
descfile=${eventfile}

eventfile_path="${eventfile}"
descfile_path="${descfile}"

if [ ! -f "${eventfile_path}" ] ; then
	echo "given event file does not exist"
	usage
	exit 1
fi

if [ ! -f "${descfile_path}" ] ; then
	echo "no device descriptor file found"
	exit 1
fi

TS_VERIFY=$(readlink -f ../ts_verify)

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

    $TS_VERIFY &

    # give tslib some time to start up
    sleep 1

    evemu-play $word < "${eventfile_path}"

  done
done
}
