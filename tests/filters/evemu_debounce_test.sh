#!/bin/bash
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo "build tslib before running this in tests/filters/"
# TODO check this :)

export TSLIB_CONFFILE=$(readlink -f evemu_debounce_tsconf)
# TODO get the ts.conf on the commandline

echo ts.conf: $TSLIB_CONFFILE
echo ==============
cat $TSLIB_CONFFILE
echo ==============

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
      echo -e "example: ${GREEN}evemu-play $word < evemu_maxtouch_rec_debounce${NC}"
      $TS_PRINT_MT
  done
done
}
