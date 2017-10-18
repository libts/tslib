## tslib filter plugin tests
This directory contains scripts and recordings in order to test our filters
without the need for real touchscreen devices, using [Evemu](https://www.freedesktop.org/wiki/Evemu/).

These tests are used during development and documented here as they are not
included in tslib's release tarballs.

### How-to


		./evemu_test.sh -f <filtername> -e <eventfile>


These tests use the [evemu-devices](https://github.com/whot/evemu-devices)
repository with it's collection of device descriptors and recordings. It is
cloned as part of the test script.

If you want to permanently add a recording you want to test against filters,
you have to add it there.

Some files involved in a filter test are:
* `<filter_name>.conf` - ts.conf config file for tslib, including the filter plugin
* `<eventfile>.<filter_name>.expected` - the expected filtered result for the
recording. "eventfile" is a events filename from the
[evemu-devices](https://github.com/whot/evemu-devices) repository
