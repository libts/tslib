## tslib filter plugin tests
This directory contains scripts and recordings in order to test our filters
without the need for real touchscreen devices, using [Evemu](https://www.freedesktop.org/wiki/Evemu/).

These tests are used during development and documented here as they are not
included in tslib's release tarballs.

### How to test a filter


		./test.sh -f <filtername> -e "<eventfile>"


These tests use the [evemu-devices](https://github.com/whot/evemu-devices)
repository with it's collection of device descriptors and recordings. It is
cloned as part of the test script.

If you want to permanently add a recording you want to test against filters,
you have to add it there.

Some files involved in a filter test are:
* `<filter_name>.conf` - ts.conf config file for tslib, including the filter plugin
* `<eventfile>.<filter_name>.expected` - the expected filtered result for the
recording. "eventfile" is a events filename from the
[evemu-devices](https://github.com/whot/evemu-devices/tree/master/touchscreens)
repository: NAME.DESC.events

Example:

		./test.sh -f iir -e "Atmel maXTouch Touchscreen.4-finger-drag-down.events"


Use a filter / eventfile combination for that an ".expected" file is available
here.

#### Creating .expected files

Creating the files included here, with suffix "expected", requires attention.
They enable to run tests in the first place.

They should be known-good files to run the test against. Don't just copy
a random result-file. In order to verify a filter, first create specific
recordings, not included here, possibly with "errors", in order to see the
filter do it's job. Only when manually verified, run it on the various
recordings available.

As soon as you want to add ".expected" files where other filter parameters
than in the current ".conf" files are being used, you have to create a new
"filtername_new.conf" file and use that to call `test.sh -f <filtername_new>`

### How to test the libts API

`ts_verify_evemu.sh` uses evemu and a recording saved here, named
"NAME.ts-verify-X.events" to run ts_verify against libts.

		./ts_verify_evemu.sh -e "<eventfile>"


For example

		./ts_verify_evemu.sh -e "Atmel maXTouch Touchscreen.ts-verify-1.events"

