![logo](website/logo.svg)
 [![Coverity Scan Build Status](https://scan.coverity.com/projects/11027/badge.svg)](https://scan.coverity.com/projects/tslib)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/libts/tslib.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/libts/tslib/context:cpp)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/752/badge)](https://bestpractices.coreinfrastructure.org/projects/752)

# C library for filtering touchscreen events

tslib consists of the library _libts_ and tools that help you _calibrate_ and
_use it_ in your environment.

## contact
If you have problems, questions, ideas, or suggestions, please contact us by
writing an email to [tslib@lists.infradead.org](mailto:tslib@lists.infradead.org),
which is [our mailing list](http://lists.infradead.org/mailman/listinfo/tslib).

## website
Visit the [tslib website](http://tslib.org) for an overview of the project.

## table of contents
* [setup and configure tslib](#setup-and-configure-tslib)
* [filter modules](#filter-modules)
* [the libts library](#the-libts-library)
* [building tslib](#building-tslib)
* [hardware support](#touchscreen-hardware-support)
* [contribute](#contributors)


## setup and configure tslib
### install tslib
tslib runs on various hardware architectures and operating systems, including GNU/Linux,
FreeBSD, or Android/Linux. See [building tslib](#building-tslib) for details.
Apart from building the latest tarball release, running
`./configure`, `make`, and `make install`, tslib is available from distributors like
[Arch Linux](https://www.archlinux.org/packages/?q=tslib) / [Arch Linux ARM](https://archlinuxarm.org/packages/aarch64/tslib),
[Buildroot](https://buildroot.org/),
[Debian](https://tracker.debian.org/pkg/tslib) / [Ubuntu](https://launchpad.net/ubuntu/+source/tslib),
[Fedora](https://apps.fedoraproject.org/packages/tslib/), or
[OpenSUSE](https://software.opensuse.org/package/tslib)
and their package management.

### configure tslib
This is just an example `/etc/ts.conf` file. Touch samples flow from top to
bottom. Each line specifies one module and its parameters. Modules are
processed in order. Use _one_ `module_raw` on top, that accesses your device,
followed by any combination of filter modules.

    module_raw input
    module median depth=3
    module dejitter delta=100
    module linear

See the [section below](#filter-modules) for available filters and their
parameters. On Linux, your first commented-in line should always be
`module_raw input` which offers one optional parameter: `grab_events=1`
if you want it to execute `EVIOCGRAB` on the device.

With this configuration file, we end up with the following data flow
through the library:

    driver --> raw read --> median  --> dejitter --> linear --> application (using `ts_read_mt()`)
               module       module      module       module

### calibrate the touch screen
Calibration is done by the `linear` plugin, which uses its own config file
`/etc/pointercal`. Don't edit this file manually. It is created by the
[`ts_calibrate`](https://manpages.debian.org/unstable/libts0/ts_calibrate.1.en.html) program:

    # ts_calibrate

The calibration procedure simply requires you to touch a cross on the screen,
where it appears, as accurately as possible.

![ts_calibrate](doc/screenshots/ts_calibrate.png?raw=true)

### test the filtered input behaviour
You may quickly test the touch behaviour that results from the configured
filters, using [`ts_test_mt`](https://manpages.debian.org/unstable/libts0/ts_test_mt.1.en.html):

    # ts_test_mt

![ts_test_mt](doc/screenshots/ts_test_mt.png?raw=true)

On the bottom left of the screen, you will see the available concurrent touch contacts
supported, and whether it's because the driver says so, or `ts_test_mt` was started
with the `-j` commandline option to overwrite it.

### environment variables (optional)
You may override the defaults. In most cases, though, you won't need to do so:

    TSLIB_TSDEVICE          Touchscreen device file name.
                            Default:                automatic detection (on Linux)

    TSLIB_CALIBFILE         Calibration file.
                            Default:                ${sysconfdir}/pointercal

    TSLIB_CONFFILE          Config file.
                            Default:                ${sysconfdir}/ts.conf

    TSLIB_PLUGINDIR         Plugin directory.
                            Default:                ${datadir}/plugins

    TSLIB_CONSOLEDEVICE     Console device. (not needed when using --with-sdl2)
                            Default:                /dev/tty

    TSLIB_FBDEVICE          Framebuffer device.
                            Default:                /dev/fb0

### use the filtered result in your system (X.org method)
If you're using X.org graphical X server, things should be very easy. Install
tslib and [xf86-input-tslib](https://github.com/merge/xf86-input-tslib),
reboot, and you should instantly have your `ts.conf` filters running, without
configuring anything else yourself.

### use the filtered result in your system (ts_uinput method)
**TL;DR:** Run `tools/ts_uinput_start.sh` during startup and use
`/dev/input/ts_uinput` as your evdev input device.


tslib tries to automatically find your touchscreen device in `/dev/input/event*`
on Linux. Now make `ts_uinput` use it, instead of your graphical environment
directly:

    # ts_uinput -d -v

`-d` makes the program return and run as a daemon in the background. `-v` makes
it print the __new__ `/dev/input/eventX` device node before returning.

Now make your graphical environment use that new input device, using **evdev**
drivers.

* For *Qt5* for example you'd probably set something like this:

    QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS=/dev/input/eventX:rotate=0

* For *X11* you'd probably edit your `xorg.conf` `Section "InputDevice"` for your
touchscreen to have

    Option "Device" "/dev/input/eventX"

Please consult your input system's documentation on how to use a specific
evdev input device.

Let's recap the data flow here:

    driver --> raw read --> filter(s) ... --> ts_uinput --> libevdev read  --> GUI app/toolkit
               module       module(s) ...     daemon        e.g. in libinput

#### symlink /dev/input/ts_uinput to the new event file
/dev/input/event numbers are not persistent. In order to know in advance,
*what* enumerated input device file is created by `ts_uinput`, you can use
a symlink:

* use the included `tools/ts_uinput_start.sh` script that starts
  `ts_uinput -d -v` and creates the symlink called `/dev/input/ts_uinput` for
  you, or

* if you're using *systemd*, create the following udev rule, for
  example `/etc/udev/rules.d/98-touchscreen.rules`:

      SUBSYSTEM=="input", KERNEL=="event[0-9]*", ATTRS{name}=="NAME_OF_THE_TOUCH_CONTROLLER", SYMLINK+="input/ts", TAG+="systemd" ENV{SYSTEMD_WANTS}="ts_uinput.service"
      SUBSYSTEM=="input", KERNEL=="event[0-9]*", ATTRS{name}=="ts_uinput", SYMLINK+="input/ts_uinput"

where `NAME_OF_THE_TOUCH_CONTROLLER` the touchscreen found in your `cat /proc/bus/input/devices | grep Name`. The first rule is only needed, if tslib doesn't automatically choose
the correct device for you.

#### running as systemd service (optional)
In case you have to use non-default paths, create a file containing the
environment for tslib, like `/etc/ts.env`

      TSLIB_CALIBFILE=/etc/pointercal
      TSLIB_CONFFILE=/etc/ts.conf
      TSLIB_PLUGINDIR=/usr/lib/ts

and create a systemd service file, such as `/usr/lib/systemd/system/ts_uinput.service`

      [Unit]
      Description=touchscreen input
      BindsTo=dev-input-ts.device
      After=dev-input-ts.device
      RequiresMountsFor=/etc/ts.env

      [Service]
      Type=forking
      EnvironmentFile=/etc/ts.env
      ExecStart=/usr/bin/ts_uinput -d

Adjust the paths. They could just as well be in `/usr/local/`, too.

### other operating systems
There is no tool that we know of that reads tslib samples and uses the
[Windows touch injection API](https://msdn.microsoft.com/en-us/library/windows/desktop/hh802899(v=vs.85).aspx),
for example (yet).


## filter modules

### module: linear
  Linear scaling - calibration - module, primerily used for conversion of touch
  screen co-ordinates to screen co-ordinates. It applies the corrections as
  recorded and saved by the [`ts_calibrate`](https://manpages.debian.org/unstable/libts0/ts_calibrate.1.en.html) tool. It's the only module that reads
  a configuration file.

Parameters (usually not needed):
* `rot`

	overwrite the rotation to apply. Clockwise: `rot=1`, upside down:
	`rot=2`, counter-clockwise: `rot=3`. Default: screen-rotation during
	`ts_calibrate` calibration.

* `xyswap`

	Interchange the X and Y co-ordinates -- no longer used or needed
	if the linear calibration utility `ts_calibrate` is used.

* `pressure_offset`

	Set the offset applied to the pressure value. Default: 0

* `pressure_mul`

	factor to multiply the pressure value by. Default: 1.
* `pressure_div`

	value to divide the pressure value by. Default: 1.

Example: `module linear rot=0`


### module: invert
  Invert values in the X and/or Y direction around the given value.
  There are no default values. If specified, a value has to be
  set. If one axis is not specified, it's simply untouched.

Parameters:
* `x0`

	X-axis (horizontal) value around which to invert.
* `y0`

	Y-axis (vertical) value around which to invert.

Example: `module invert y0=640` (Y-axis inverted for 640 screen height, X-axis untouched)


### module: median
  The median filter reduces noise in the samples' coordinate values. It is
  able to filter undesired single large jumps in the signal. For some
  theory, see [Wikipedia](https://en.wikipedia.org/wiki/Median_filter)

Parameters:
* `depth`

	Number of samples to apply the median filter to. Default: 3.

Example: `module median depth=5`


### module: pthres
  Pressure threshold filter. Given that a release is always pressure 0 and a
  press is always >= 1, this discards samples below / above the specified
  pressure threshold.

Parameters:
* `pmin`

	Minimum pressure value for a sample to be valid. Default: 1.
* `pmax`

	Maximum pressure value for a sample to be valid. Default: (`INT_MAX`).

Example: `module pthres pmin=10`


### module: iir
  Infinite impulse response filter. This is a smoothing filter to remove
  low-level noise. There is a trade-off between noise removal
  (smoothing) and responsiveness. The parameters N and D specify the level of
  smoothing in the form of a fraction (N/D).

  [Wikipedia](https://en.wikipedia.org/wiki/Infinite_impulse_response) has some
  theory.

Parameters:
* `N`

	numerator of the smoothing fraction. Default: 0.
* `D`

	denominator of the smoothing fraction. Default: 1.

Example: `module iir N=6 D=10`


### module: dejitter
  Removes jitter on the X and Y co-ordinates. This is achieved by applying a
  weighted smoothing filter. The latest samples have most weight; earlier
  samples have less weight. This allows one to achieve 1:1 input->output rate. See
  [Wikipedia](https://en.wikipedia.org/wiki/Jitter#Mitigation) for some
  theory.

Parameters:
* `delta`

	Squared distance between two samples ((X2-X1)^2 + (Y2-Y1)^2) that
	defines the 'quick motion' threshold. If the pen moves quick, it
	is not feasible to smooth pen motion, besides quick motion is not
	precise anyway; so if quick motion is detected the module just
	discards the backlog and simply copies input to output. Default: 100.

Example: `module dejitter delta=100`


### module: debounce
  Simple debounce mechanism that drops input events for the specified time
  after a touch gesture stopped. [Wikipedia](https://en.wikipedia.org/wiki/Switch#Contact_bounce)
  has more theory.

Parameters:
* `drop_threshold`

	drop events up to this number of milliseconds after the last
	release event. Default: 0.

Example: `module debounce drop_threshold=40`


### module: skip
  Skip `nhead` samples after press and `ntail` samples before release. This
  should help if the first or last samples are unreliable for the device.

Parameters:
* `nhead`

	Number of events to drop after pressure. Default: 1.
* `ntail`

	Number of events to drop before release. Default: 1.

Example: `module skip nhead=2 ntail=1`


### module: lowpass
  Simple lowpass exponential averaging filtering module.

Parameters:
* `factor`

	floating point value between 0 and 1; for example 0.2 for more smoothing
	or 0.8 for less. Default: 0.4.
* `threshold`

	x or y minimum distance between two samples to start applying the
        filter. Default: 2.

Example: `module lowpass factor=0.5 threshold=1`


### module: evthres
  After "pen/finger down", N number of input samples need to be delivered by the
  driver before they are considered valid and passed on to the user
  (application). If "pen/finger up" occurs before N samples are being
  read from the device driver, tslib will drop the "tap".

  This filter can be used to avoid touches that, for example, result from
  electromagnetic interference. These are known to be shorter than one a real
  user would create.

  In contrast to the `skip` filter, the `evthres` filter will not cut out
  events that are part of a real touch input. It will only cut out one
  whole "tap", if short enough.

  Compared to the `debounce` filter, this filter will act on every occurrence
  of "pen/finger down", including the first one, not only starting with the
  second. Also, `debounce` is time-based, not events-based.

Parameters:
* `N`

	number of events between "down" and "up" that must be provided
	by the device driver for the touch to be considered real and passed on.

Example: `module evthres N=5`


### module:	variance
  Variance filter. Tries to do its best in order to filter out random noise
  coming from touchscreen ADC's. This is achieved by limiting the sample
  movement speed to some value (e.g. the pen is not supposed to move quicker
  than some threshold).

  There is **no multitouch** support for this filter (yet). `ts_read_mt()` will
  limit your input to one slot when this filter is used. Try using the median
  filter instead.

Parameters:
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



***
The following example setup

           |--------|       |-----|      |--------------|
    x ---> | median | ----> | IIR | ---> |              | ---> x'
           |--------|    -> |-----|      |    screen    |
                        |                |  transform   |
                        |                | (calibrate)  |
           |--------|   |   |-----|      |              |
    y ---> | median | ----> | IIR | ---> |              | ---> y'
           |--------|   |-> |-----|      |--------------|
                        |
                        |
                 |----------|
    p ---------> | debounce | -------------------------------> p'
                 |----------|

would be achieved by the following ts.conf:

    module_raw input
    module debounce drop_threshold=40
    module median depth=5
    module iir N=6 D=10
    module linear

while you are free to play with the parameter values.

### screen rotation
The graphical tools support rotating the screen, see
`ts_calibrate --help` or the man pages for the details.
`ts_calibrate` will (in the background) ignore screen rotation
but will save the current rotation state in the "calibration file"
`TSLIB_CALIBFILE`. The `linear` filter module will pick that up
and apply the given rotation. It can be overwritten by using
the `rot` module parameter: `module linear rot=0` (if your
toolkit or higher level application does rotation).

***


## the libts library
### the libts API
The API is documented in our man pages in the doc directory.
Check out our tests directory for examples how to use it.

[`tslib_version()`](https://manpages.debian.org/unstable/libts0/tslib_version.3.en.html)  
[`ts_libversion()`](https://manpages.debian.org/unstable/libts0/ts_libversion.3.en.html)  
[`ts_open()`](https://manpages.debian.org/unstable/libts0/ts_open.3.en.html)  
[`ts_config()`](https://manpages.debian.org/unstable/libts0/ts_config.3.en.html)  
[`ts_setup()`](https://manpages.debian.org/unstable/libts0/ts_setup.3.en.html)  
[`ts_close()`](https://manpages.debian.org/unstable/libts0/ts_close.3.en.html)  
[`ts_reconfig()`](https://manpages.debian.org/unstable/libts0/ts_config.3.en.html)  
`ts_option()`  
[`ts_fd()`](https://manpages.debian.org/unstable/libts0/ts_fd.3.en.html)  
`ts_load_module()`  
[`ts_read()`](https://manpages.debian.org/unstable/libts0/ts_read.3.en.html)  
[`ts_read_raw()`](https://manpages.debian.org/unstable/libts0/ts_read.3.en.html)  
[`ts_read_mt()`](https://manpages.debian.org/unstable/libts0/ts_read.3.en.html)  
[`ts_read_raw_mt()`](https://manpages.debian.org/unstable/libts0/ts_read.3.en.html)  
[`int (*ts_error_fn)(const char *fmt, va_list ap)`](https://manpages.debian.org/unstable/libts0/ts_error_fn.3.en.html)  
[`int (*ts_open_restricted)(const char *path, int flags, void *user_data)`](https://manpages.debian.org/unstable/libts0/ts_open_restricted.3.en.html)  
[`void (*ts_close_restricted)(int fd, void *user_data)`](https://manpages.debian.org/unstable/libts0/ts_close_restricted.3.en.html)  
[`ts_get_eventpath()`](https://manpages.debian.org/unstable/libts0/ts_get_eventpath.3.en.html)  
[`ts_print_ascii_logo()`](https://manpages.debian.org/unstable/libts0/ts_print_ascii_logo.3.en.html)  
[`ts_conf_get()`](https://manpages.debian.org/unstable/libts0/ts_conf_get.3.en.html)  
[`ts_conf_set()`](https://manpages.debian.org/unstable/libts0/ts_conf_set.3.en.html)  


### using libts
To use the library from C or C++, include the following preprocessor directive
in your source files:

    #include <tslib.h>


To link with the library, specify `-lts` as an argument to the linker.

#### compiling using autoconf and pkg-config
On UNIX systems, you can use `pkg-config` to automatically select the appropriate
compiler and linker switches for libts. The `PKG_CHECK_MODULES` m4 macro may be
used to automatically set the appropriate Makefile variables:

    PKG_CHECK_MODULES([TSLIB], [tslib >= 1.10],,
      AC_MSG_ERROR([libts 1.10 or newer not found.])
    )


If you want to support tslib < 1.2, while still supporting multitouch and all
recent versions of tslib, you'd do something like this:

    #include <tslib.h>

    #ifndef TSLIB_VERSION_MT
            /* ts_read() as before (due to old tslib) */
    #else
            /* new ts_setup() and ret = ts_read_mt() */
            if (ret == -ENOSYS)
                    /* ts_read() as before (due to user config) */
    #endif

This is a complete example program, similar to `ts_print_mt.c`:

    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <fcntl.h>
    #include <sys/time.h>
    #include <unistd.h>
    #include <errno.h>

    #include <tslib.h>

    #define SLOTS 5
    #define SAMPLES 1

    int main(int argc, char **argv)
    {
        struct tsdev *ts;
        char *tsdevice = NULL;
        struct ts_sample_mt **samp_mt = NULL;
        int ret, i, j;

        ts = ts_setup(tsdevice, 0);
        if (!ts) {
                perror("ts_setup");
                return -1;
        }

        samp_mt = malloc(SAMPLES * sizeof(struct ts_sample_mt *));
        if (!samp_mt) {
                ts_close(ts);
                return -ENOMEM;
        }
        for (i = 0; i < SAMPLES; i++) {
                samp_mt[i] = calloc(SLOTS, sizeof(struct ts_sample_mt));
                if (!samp_mt[i]) {
                        for (i--; i >= 0; i--)
                                free(samp_mt[i]);
                        free(samp_mt);
                        ts_close(ts);
                        return -ENOMEM;
                }
        }

        while (1) {
                ret = ts_read_mt(ts, samp_mt, SLOTS, SAMPLES);
                if (ret < 0) {
                        perror("ts_read_mt");
                        ts_close(ts);
                        exit(1);
                }

                for (j = 0; j < ret; j++) {
                	for (i = 0; i < SLOTS; i++) {
			#ifdef TSLIB_MT_VALID
				if (!(samp_mt[j][i].valid & TSLIB_MT_VALID))
					continue;
			#else
				if (samp_mt[j][i].valid < 1)
					continue;
			#endif

				printf("%ld.%06ld: (slot %d) %6d %6d %6d\n",
				       samp_mt[j][i].tv.tv_sec,
				       samp_mt[j][i].tv.tv_usec,
				       samp_mt[j][i].slot,
				       samp_mt[j][i].x,
				       samp_mt[j][i].y,
				       samp_mt[j][i].pressure);
                        }
                }
        }

        ts_close(ts);
    }


If you know how many slots your device can handle, you could avoid malloc:

    struct ts_sample_mt TouchScreenSamples[SAMPLES][SLOTS];

    struct ts_sample_mt (*pTouchScreenSamples)[SLOTS] = TouchScreenSamples;
    struct ts_sample_mt *ts_samp[SAMPLES];
    for (i = 0; i < SAMPLES; i++)
            ts_samp[i] = pTouchScreenSamples[i];

and call `ts_read_mt()` like so

    ts_read_mt(ts, ts_samp, SLOTS, SAMPLES);


### ABI - Application Binary Interface

[Wikipedia](https://en.wikipedia.org/wiki/Application_binary_interface) has
background information.

#### libts Soname versions
Usually, and every time until now, libts does not break the ABI and your
application can continue using libts after upgrading. Specifically this is
indicated by the libts library version's major number, which should always stay
the same. According to our versioning scheme, the major number is incremented
only if we break backwards compatibility. The second or third minor version will
increase with releases. In the following example


    libts.so -> libts.so.0.7.0
    libts.so.0 -> libts.so.0.7.0
    libts.so.0.7.0


use `libts.so` for using tslib unconditionally and `libts.so.0` to make sure
your current application never breaks.

If a release includes changes such as added features, the second number is
incremented and the third is set to zero. If a release includes mostly just
bugfixes, only the third number is incremented.

#### tslib package version
A tslib tarball version number doesn't tell you anything about its backwards
compatibility.

### dependencies

* libc (with libdl only when building dynamically linked)
* libsdl2-dev (only when using `--with-sdl2` for [SDL2](https://www.libsdl.org/) graphical applications)

### related libraries

* [libevdev](https://www.freedesktop.org/wiki/Software/libevdev/) - access wrapper for [event device](https://en.wikipedia.org/wiki/Evdev) access, uinput too ("Linux API")
* [libinput](https://www.freedesktop.org/wiki/Software/libinput/) - handle input devices for Wayland (uses libevdev)
* [xf86-input-evdev](https://cgit.freedesktop.org/xorg/driver/xf86-input-evdev/) - evdev plugin for X (uses libevdev)

### libts users
This lists the programs for the *every day use* of tslib, facing the outside world.
For testing purposes there are tools like [ts_test_mt](#test-the-filtered-input-behaviour)
too.

#### shipped as part of tslib
* [ts_calibrate](#filter-modules) - graphical calibration tool. Configures the `linear` filter module.
* [ts_uinput](#use-the-filtered-result-in-your-system-ts_uinput-method) - userspace **evdev** driver for the tslib-filtered samples.

#### third party applications
* [xf86-input-tslib](https://github.com/merge/xf86-input-tslib) - direct tslib input driver for X11
* [qtslib](https://github.com/qt/qtbase/tree/dev/src/platformsupport/input/tslib) - direct Qt5 tslib input plugin
* [enlightenment](https://www.enlightenment.org/) - A Window Manager (direct support in framebuffer mode, X11 via xf86-input-tslib)
* [DirectFB](https://github.com/deniskropp/DirectFB) - Graphics library on top of framebuffer

### tslib module API
`struct tslib_module_info`  
`struct tslib_vars`  
`struct tslib_ops`  
`tslib_parse_vars(struct tslib_module_info *,const struct tslib_vars *, int, const char *);`  

tslib modules (filter or driver/raw module) in the plugins directory need to
implement `mod_init()`. If the module takes parameters, it has to declare a
`const struct tslib_vars` and pass that, its lengths and the params string
that is passed to `mod_init` to `tslib_parse_vars()` during `mod_init()`.

Furthermore a `const struct tslib_ops` has to be declared, with its members
pointing to the module's implementation of module-operations like `read_mt`
that get called in the chain of filters.


### Symbols in Versions
|Name | Introduced|
| --- | --- |
|`TSLIB_VERSION_MT` | 1.10 |
|`TSLIB_VERSION_OPEN_RESTRICTED` | 1.13 |
|`TSLIB_VERSION_EVENTPATH` | 1.15 |
|`TSLIB_VERSION_VERSION` | 1.16 |
|`TSLIB_MT_VALID` | 1.13 |
|`TSLIB_MT_VALID_TOOL` | 1.13 |
|`tslib_version` | 1.16 |
|`ts_print_ascii_logo` | 1.16 |
|`ts_libversion` | 1.10 |
|`ts_close` | 1.0 |
|`ts_config` | 1.0 |
|`ts_reconfig` | 1.3 |
|`ts_setup` | 1.4 |
|`ts_error_fn` | 1.0 |
|`ts_open_restricted` | 1.13 |
|`ts_close_restricted` | 1.13 |
|`ts_fd` | 1.0 |
|`ts_load_module` | 1.0 |
|`ts_open` | 1.0 |
|`ts_option` | 1.1 |
|`ts_read` | 1.0 |
|`ts_read_mt` | 1.3 |
|`ts_read_raw` | 1.0 |
|`ts_read_raw_mt` | 1.3 |
|`tslib_parse_vars` | 1.0 |
|`ts_get_eventpath` | 1.15 |
|`ts_conf_get` | 1.18 |
|`ts_conf_set` | 1.18 |


***

## building tslib

### shared vs. static builds

libts can be built to fit your needs. Use the configure script to enable only
the modules you need. By default, libts is built as a shared library, with
each module being a shared library object itself. You can, however, configure
tslib to build libts statically linked, and the needed modules compiled inside
of libts. Here's an example for this:

    ./configure --enable-static --disable-shared --enable-input=static --enable-linear=static --enable-iir=static

This should result in a `libts.a` of roughly 50 kilobytes, ready for using
calibration (linear filter) and the infinite impulse response filter in ts.conf.

### CMake

Alternatively, you can use CMake to build the project.
To create building in the project tree:

    mkdir build && cd build
    cmake ../
    # or adding configuration options: cmake -Denable-input-evdev=ON ../
    cmake --build .
    cmake -P cmake_install.cmake

By default, the core tslib is built as a shared library.
In order to build it as static, add `-DBUILD_SHARED_LIBS=OFF` to the configure line.

Also, the plugins are by default built as shared.  Add `-Dstatic-<module>=ON` to the configuration step to
build plugins statically into the core tslib. To disable and enable modules, 
use flags: `-Denable-<module>=ON/OFF`.

#### Using tslib in client apps

The following is a minimal example of how to use tslib, built with CMake, in your client app. 
Adding `tslib::tslib` as a link target will add required dependencies and include directories generated build files.

    cmake_minimum_required(VERSION 3.10)
    project(tslib_client)
    find_package(tslib 1.16)
    add_executable(tslib_client main.c)
    target_link_libraries(tslib_client PUBLIC tslib::tslib)
	

### portable `ts_calibrate` and `ts_test_mt` using SDL2

In case you cannot draw directly on the framebuffer, there is an __experimental__
implementation of the necessary graphical tools using SDL2. They are more portable,
but require more resources to run. To use them, make sure you have SDL2 and the
development headers installed and use `./configure --with-sdl2`.

### portability

tslib is cross-platform; you should be able to build it on a large variety of
operating systems.

#### libts and filter plugins (`module`)

This is the hardware independent core part: _libts and all filter modules_ as
_shared libraries_, build on the following operating systems and probably more.

* **GNU / Linux**
* **Android / Linux**
* **FreeBSD**
* **GNU / Hurd**
* **Haiku**
* **Windows**
* **Mac OS X**

#### input plugins (`module_raw`)

This makes the thing usable in the real world because it accesses your device.
See [hardware support](#touchscreen-hardware-support) for the currently
possible configuration for your platform.

The libts default configuration currently has the following input modules
__disabled__:
* `cy8mrln-palmpre`
* `dmc_dus3000`
* `galax`
* `arctic2`
* `corgi`
* `collie`
* `dmc`
* `h3600`
* `mk712`
* `ucb1x00`
* `tatung`

Please note that this list may grow over time. If you rely on
a particular input plugin, you should enable it explicitly. On Linux,
you should only need `input` though.

* GNU / Linux - all (most importantly `input`)
  - `./configure`
* Android / Linux - all (most importantly `input`)
  - `./configure`
* FreeBSD - almost all (most importantly `input`)
  - `./configure --disable-waveshare`
* GNU / Hurd - some, see [hardware support](#touchscreen-hardware-support)
  - `./configure --disable-input --disable-waveshare`
* Haiku - some, see [hardware support](#touchscreen-hardware-support)
  - `./configure --disable-input --disable-touchkit --disable-waveshare`
* Windows - no tslib module for the [Windows touchscreen API](https://msdn.microsoft.com/en-us/library/windows/desktop/dd317323(v=vs.85).aspx) (yet)
  - `./configure --with-sdl2 --disable-input --disable-touchkit --disable-waveshare`

Writing your own plugin is quite easy, in case an existing one doesn't fit.

#### test programs and tools

* GNU / Linux - all
* Android / Linux - all (?)
* FreeBSD - all
* GNU / Hurd - ts_print_mt, ts_print, ts_print_raw, ts_finddev
* Haiku - ts_print_mt, ts_print, ts_print_raw, ts_finddev
* Windows - ts_print.exe, ts_print_raw.exe ts_print_mt.exe ts_test_mt.exe ts_calibrate.exe

#### download binaries?
For GNU/Linux all architectures are _very_ well covered, thanks to Debian, Arch
Linux, and others.

Please help porting missing programs!

## touchscreen hardware support
**TL;DR:** On Linux, use `module_raw input`

For mostly historical reasons, tslib includes device specific `module_raw` userspace
drivers.
The [ts.conf man page](https://manpages.debian.org/unstable/libts0/ts.conf.5.en.html)
has details on the available `module_raw` drivers; not all of them are listed in the
default `etc/ts.conf` config file. Those are to be considered workarounds and may get
disabled in the default configuration in the future.
If you use one of those, please `./configure --enable-...` it explicitly.

It is strongly recommended to have a real device driver for your system
and use a generic access `module_raw` of tslib. For Linux ([evdev](https://en.wikipedia.org/wiki/Evdev))
this is called `input`. There is an equivalent experimental module that needs libevdev
installed: `module_raw input_evdev`.

## Contributors

This project exists [[thanks](THANKS)] to all the people who contribute. [[Contribute](CONTRIBUTING.md)].
<a href="https://github.com/libts/tslib/contributors"><img src="https://opencollective.com/tslib/contributors.svg?width=890&button=false" /></a>

## support this project

In case you like our project, use it in your professional environment,
and want it to stay maintained, please consider [sponsoring the current
maintainer via Github](https://github.com/sponsors/merge) (or ask for
a btc address if that's what you do). Maintenance costs time and money.
thank you very much.

## Sponsors

none. [become a sponsor](https://github.com/sponsors/merge) and be listed here.
