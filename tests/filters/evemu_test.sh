#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:
filtername=""
verbose=0

while getopts "h?vf:" opt; do
    case "$opt" in
    h|\?)
	echo "Usage: ./evemu_test -f <filtername>"
        exit 0
        ;;
    v)  verbose=1
        ;;
    f)  filtername=$OPTARG
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift


# trap ctrl-c and call ctrl_c()
trap ctrl_c INT

function ctrl_c() {
	diff --report-identical-files ${filtername}.expected result/${filtername}.result
}


if [ -z "$filtername" ] ; then
	echo "Usage: ./evemu_test -f <filtername>"
	exit 1
fi

export TSLIB_CONFFILE=$(readlink -f evemu_${filtername}_tsconf)
# TODO get the ts.conf on the commandline

if [ $verbose = 1 ] ; then
	echo -e "${YELLOW}ts.conf:${NC}"
	cat $TSLIB_CONFFILE
	echo ""
fi

EVEMU_DESC=$(readlink -f evemu_maxtouch_desc)
TS_PRINT_MT=$(readlink -f ../ts_print_mt)

mkdir -p result

# create the virtual touchscreen via uinput
evemu-device $EVEMU_DESC | {
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
      echo -e "running ${GREEN}evemu-play $word < evemu_maxtouch_rec_${filtername}${NC}"
      echo "please verify the resulting samples you see and quit"
    fi

    $TS_PRINT_MT | grep sample | sed --expression='s/[[:digit:]]\+\.\+[[:digit:]]\+//g' > result/${filtername}.result &

    # give tslib some time to start up
    sleep 1

    evemu-play $word < evemu_maxtouch_rec_${filtername}

  done
done
}
