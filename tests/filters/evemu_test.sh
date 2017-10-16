#!/bin/bash
 set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
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

if [ $verbose = 1 ] ; then
	echo "verbose=$verbose, filter='$filtername', Leftovers: $@"
fi

if [ -z "$filtername" ] ; then
	echo "Usage: ./evemu_test -f <filtername>"
	exit 1
fi

if [ $verbose = 1 ] ; then
	echo "build tslib before running this in tests/filters/"
fi

export TSLIB_CONFFILE=$(readlink -f evemu_${filtername}_tsconf)
# TODO get the ts.conf on the commandline

if [ $verbose = 1 ] ; then
	echo "===== ts.conf: $TSLIB_CONFFILE ====="
	cat $TSLIB_CONFFILE
	echo "===================================="
fi

EVEMU_DESC=$(readlink -f evemu_maxtouch_desc)
TS_PRINT_MT=$(readlink -f ../ts_print_mt)

# create the virtual touchscreen via uinput
evemu-device $EVEMU_DESC | {
# this only tries to catch the created device path
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
      echo "evemu device: $word"
      export TSLIB_TSDEVICE=$word
      echo "now play a sequence to see the filtered result"
      echo -e "example: ${GREEN}evemu-play $word < evemu_maxtouch_rec_${filtername}${NC}"
      $TS_PRINT_MT
  done
done
}
