## tslib filter plugin tests
This directory contains scripts and recordings in order to test our filters
without the need for real touchscreen devices, using [Evemu](https://www.freedesktop.org/wiki/Evemu/).

These tests are used during development and documented here as they are not
included in tslib's release tarballs.

### Usage


		./evemu_test.sh -f <filtername>


### file conventions
For each touchscreen device used in evemu recordings, there must be one evemu
descriptor file:
* `evemu_<touchscreen_name>_desc` - evemu touchscreen descriptor file

Each filter to be tested conists of 3 files:
* `evemu_<filter_name>_tsconf` - ts.conf config file for tslib
* `evemu_<touchscreen_name>_rec_<filter_name>` - evemu touch recording for the filter
* `<filter_name>.expected` - the expected filtered result for the recording

### available filters to test
* `debounce`
* `skip`

### available touchscreens
* `maxtouch`
