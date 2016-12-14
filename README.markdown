# tslib

Touchscreen access library

[![Coverity Scan Build Status](https://scan.coverity.com/projects/11027/badge.svg)](https://scan.coverity.com/projects/tslib)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/4d2b6d4f5cfd4e1e8902655846d79266)](https://www.codacy.com/app/martink/tslib?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=kergoth/tslib&amp;utm_campaign=Badge_Grade)

[![Github Downloads 1.2](https://img.shields.io/github/downloads/kergoth/tslib/1.2/total.svg)](https://github.com/kergoth/tslib/releases/tag/1.2)
[![Github Downloads 1.1](https://img.shields.io/github/downloads/kergoth/tslib/1.1/total.svg)](https://github.com/kergoth/tslib/releases/tag/1.1)

## Getting tslib

Apart from direct availability here, tslib is currently maintained by the following distributors:

* [Buildroot](https://buildroot.org/)

## General

The idea of tslib is to have a core library that provides standardised
services, and a set of plugins to manage the conversion and filtering as
needed.

The plugins for a particular touchscreen are loaded automatically by the
library under the control of a static configuration file, ts.conf.
ts.conf gives the library basic configuration information.  Each line
specifies one module, and the parameters for that module.  The modules
are loaded in order, with the first one processing the touchscreen data
first.  For example:

```
  module_raw input
  module median depth=3
  module dejitter delta=100
  module linear
```

These parameters are described below.

With this configuration file, we end up with the following data flow
through the library:

```
  raw read --> median  --> dejitter --> linear --> application
  module       module      module       module
```

You can re-order these modules as you wish, add more modules, or remove them
all together.  When you call `ts_read()`, the values you read are values that
have passed through the chain of filters and scaling conversions.  Another
call is provided, `ts_read_raw()` which bypasses all the modules and reads the
raw data directly from the device.

There are a couple of programs in the `tslib/tests` directory which give example
usages.  They are by no means exhaustive, nor probably even good examples.
They are basically the programs used to test this library.

## Use tslib via a normal input event device

Instead of using tslib's API calls, you can use `tslib/tools/ts_uinput` which
creates (via uinput) a new standard input event device you can use in your
environment. The new device provides the filtered and calibrated values and
should work with single- and multitouch devices.

## Multitouch

Similar to the mentioned `ts_read()`, `ts_read_mt()` reads one `struct ts_sample_mt`
per slot (number of possible contacts) and desired number of samples. You have
to provide slots*nr of them to hold the resulting values, see the multitouch
programs in `tslib/tests` for examples; there is, of course, `ts_read_raw_mt()` too.

`ts_read_mt()` aims to be a drop-in replacement for `ts_read()`, so you can use
it for any single touch device too, providing space for one slot.

## Environment Variables

```
TSLIB_TSDEVICE			TS device file name.
				Default (non inputapi): /dev/touchscreen/ucb1x00
				Default (inputapi): /dev/input/event0
TSLIB_CALIBFILE			Calibration file.
				Default: ${sysconfdir}/pointercal
TSLIB_CONFFILE			Config file.
				Default: ${sysconfdir}/ts.conf
TSLIB_PLUGINDIR			Plugin directory.
				Default: ${datadir}/plugins
TSLIB_CONSOLEDEVICE		Console device.
				Default: /dev/tty
TSLIB_FBDEVICE			Framebuffer device.
				Default: /dev/fb0
```

## Module Parameters

### module:	variance

#### Description:
  Variance filter. Tries to do it's best in order to filter out random noise
  coming from touchscreen ADC's. This is achieved by limiting the sample
  movement speed to some value (e.g. the pen is not supposed to move quicker
  than some threshold).

  This is a 'greedy' filter, e.g. it gives less samples on output than
  receives on input. It can cause problems on capacitive touchscreens that
  already apply such a filter.

  There is **no** multitouch support for this filter (yet). `ts_read_mt()` will
  only read one slot, when this filter is used. You can try using the median
  filter instead.

#### Parameters:
* `delta`

	Set the squared distance in touchscreen units between previous and
	current pen position (e.g. (X2-X1)^2 + (Y2-Y1)^2). This defines the
	criteria for determining whenever two samples are 'near' or 'far'
	to each other.

	Now if the distance between previous and current sample is 'far',
	the sample is marked as 'potential noise'. This doesn't mean yet
	that it will be discarded; if the next reading will be close to it,
	this will be considered just a regular 'quick motion' event, and it
	will sneak to the next layer. Also, if the sample after the
	'potential noise' is 'far' from both previously discussed samples,
	this is also considered a 'quick motion' event and the sample sneaks
	into the output stream.


### module: dejitter

#### Description:
  Removes jitter on the X and Y co-ordinates. This is achieved by applying a
  weighted smoothing filter. The latest samples have most weight; earlier
  samples have less weight. This allows to achieve 1:1 input->output rate.

#### Parameters:
* `delta`

	Squared distance between two samples ((X2-X1)^2 + (Y2-Y1)^2) that
	defines the 'quick motion' threshold. If the pen moves quick, it
	is not feasible to smooth pen motion, besides quick motion is not
	precise anyway; so if quick motion is detected the module just
	discards the backlog and simply copies input to output.


### module: linear

#### Description:
  Linear scaling module, primerily used for conversion of touch screen
  co-ordinates to screen co-ordinates.

#### Parameters:
* `xyswap`

	interchange the X and Y co-ordinates -- no longer used or needed
	if the new linear calibration utility ts_calibrate is used.

* `pressure_offset`

	offset applied to the pressure value
* `pressure_mul`

	factor to multiply the pressure value with
* `pressure_div`

	value to divide the pressure value by


### module: pthres

#### Description:
  Pressure threshold filter. Given a release is always pressure 0 and a
  press is always >= 1, this discards samples below / above the specified
  pressure threshold.

#### Parameters:
* `pmin`

	Minimum pressure value for a sample to be valid.
* `pmax`

	Maximum pressure value for a sample to be valid.


### module: debounce

#### Description:
  Simple debounce mechanism that drops input events for the specified time
  after a touch gesture stopped.

#### Parameters:
* `drop_threshold`

	drop events up to this number of milliseconds after the last
	release event.


### module: skip

#### Description:
  Skip nhead samples after press and ntail samples before release. This
  should help if for the device the first or last samples are unreliable.

Parameters:
* `nhead`

	Number of events to drop after pressure
* `ntail`

	Number of events to drop before release


### module: median

#### Description:
  Similar to what the variance filter does, the median filter suppresses
  spikes in the gesture. For some theory, see https://en.wikipedia.org/wiki/Median_filter

Parameters:
* `depth`

	Number of samples to apply the median filter to


## Module Creation Notes

For those creating tslib modules, it is important to note a couple things with
regard to handling of the ability for a user to request more than one ts event
at a time.  The first thing to note is that the lower layers may send up less
events than the user requested, because some events may be filtered out by
intermediate layers. Next, your module should send up just as many events
as the user requested in nr. If your module is one that consumes events,
such as variance, then you loop on the read from the lower layers, and only
send the events up when
1. you have the number of events requested by the user, or
2. one of the events from the lower layers was a pen release.
