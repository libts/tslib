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

Each filter to be tested conists of a pair of 2 files:
* `evemu_<filter_name>_tsconf` - ts.conf config file for tslib
* `evemu_<touchscreen_name>_rec_<filter_name>` - evemu touch recording for the filter

### available filters to test
* `debounce`
* `skip`

### available touchscreens
* `maxtouch`

### expected results
results are the samples that fall out of tslib's ts_read() API, i.e. the `ts_print_mt`
program. They depend on the parameters in ts.conf and the touch recordings (input
for tslib). For some filters we such recordings/parameters pairs that should
produce ***no samples*** to be read from tslib's ts_read() or ts_read_mt().
Other combinations should produce output. This list defines the expected test
results:

* debounce: no samples
* skip: no samples
