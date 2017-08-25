

![logo](website/tslib_logo_web.jpg?raw=true)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/11027/badge.svg)](https://scan.coverity.com/projects/tslib)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/752/badge)](https://bestpractices.coreinfrastructure.org/projects/752)

# C library for filtering touchscreen events

tslib consists of the library _libts_ and tools that help you _calibrate_ and
_use it_ in your environment. There's a [short introductory presentation from 2017](https://fosdem.org/2017/schedule/event/tslib/).

## contact
If you have problems, questions, ideas or suggestions, please contact us by
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


## setup and configure tslib
### install tslib
tslib runs on various hardware architectures and operating systems, including GNU/Linux,
FreeBSD or Android/Linux. See [building tslib](#building-tslib) for details.
Apart from building the latest tarball release, running
`./configure`, `make` and `make install`, tslib is available from distributors like
[Arch Linux](https://www.archlinux.org/packages/?q=tslib) / [Arch Linux ARM](https://archlinuxarm.org/packages/aarch64/tslib),
[Buildroot](https://buildroot.org/),
[Debian](https://tracker.debian.org/pkg/tslib) / [Ubuntu](https://launchpad.net/ubuntu/+source/tslib),
[Fedora](https://apps.fedoraproject.org/packages/tslib/) or
[OpenSUSE](https://software.opensuse.org/package/tslib)
and their package management.

### environment variables
You only need the variables in case the following defaults don't fit:

    TSLIB_TSDEVICE          Touchscreen device file name.
                            Default (ts_setup):     /dev/input/ts
                                                    /dev/input/touchscreen
                                                    /dev/input/event0
                                                    /dev/touchscreen/ucb1x00

    TSLIB_CALIBFILE         Calibration file.
                            Default:                ${sysconfdir}/pointercal

    TSLIB_CONFFILE          Config file.
                            Default:                ${sysconfdir}/ts.conf

    TSLIB_PLUGINDIR         Plugin directory.
                            Default:                ${datadir}/plugins

    TSLIB_CONSOLEDEVICE     Console device.
                            Default:                /dev/tty

    TSLIB_FBDEVICE          Framebuffer device.
                            Default:                /dev/fb0

### configure tslib
This is just an example `/etc/ts.conf` file. Touch samples flow from top to
bottom. Each line specifies one module and it's parameters. Modules are
processed in order. Use _one_ module_raw that accesses your device, followed
by any combination of filter modules.

    module_raw input
    module median depth=3
    module dejitter delta=100
    module linear

see the [section below](#filter-modules) for available filters and their
parameters.

With this configuration file, we end up with the following data flow
through the library:

    driver --> raw read --> median  --> dejitter --> linear --> application (using ts_read_mt())
               module       module      module       module

### calibrate the touch screen
Calibration is done by the `linear` plugin, which uses it's own config file
`/etc/pointercal`. Don't edit this file manually. It is created by the
[`ts_calibrate`](https://manpages.debian.org/unstable/libts0/ts_calibrate.1.en.html) program:

    # ts_calibrate

The calibration procedure simply requires you to touch the cross on screen,
where is appears, as accurate as possible.

![ts_calibrate](doc/screenshots/ts_calibrate.png?raw=true)

### test the filtered input behaviour
You may quickly test the touch behaviour that results from the configured
filters, using [`ts_test_mt`](https://manpages.debian.org/unstable/libts0/ts_test_mt.1.en.html):

    # ts_test_mt

![ts_test_mt](doc/screenshots/ts_test_mt.png?raw=true)

### use the filtered result in your system (X.org method)
If you're using X.org graphical X server, things should be very easy. Install
tslib and [xf86-input-tslib](https://github.com/merge/xf86-input-tslib),
reboot, and you should instantly have your `ts.conf` filters running, without
configuring anything else yourself.

### use the filtered result in your system (ts_uinput method)
This is a generic solution for Linux  - using tslib's included userspace input
**evdev** driver `ts_uinput`. You need to set the `TSLIB_TSDEVICE` environment
variable to point to your touchscreen device. But __don't__ use `/dev/input/eventX`;
the event numbers are not persistent. Use such a udev rule:

    SUBSYSTEM=="input", KERNEL=="event[0-9]*", ATTRS{name}=="mydrivername", SYMLINK+="input/ts", TAG+="systemd"

or find another way of creating a `/dev/input/ts` symlink. You'd need that for any
application using your device. Now you can use `ts_uinput`:

    # ts_uinput -d -v

`-d` makes the program return and run as a daemon in the background. `-v` makes
it print the __new__ `/dev/input/eventX` device node before returning.

You can use **evdev** drivers now. For *Qt5* for example you'd
probably set something like this:

    QT_QPA_GENERIC_PLUGINS=evdevtouch:/dev/input/eventX
    QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS=/dev/input/eventX:rotate=0

For *X11* you'd probably edit your `xorg.conf` `Section "InputDevice"` for your
touchscreen to have

    Option "Device" "/dev/input/eventX"

For *Wayland*, you'd make libinput use the new `/dev/input/eventX` and so on.
Please see your system's documentation on how to use a specific evdev input device.

Remember to set your environment and configuration for `ts_uinput`, just like you
did for `ts_calibrate` or `ts_test_mt`.

Let's recap the data flow here:

    driver --> raw read --> filter --> filter(s) --> ts_uinput (ts_read_mt())  --> libevdev read  --> GUI app
               module       module     module(s)     daemon                        e.g. in libinput

#### symlink to /dev/input/ts_uinput
Again, /dev/input/event numbers are not persistent. In order to know in advance,
*what* enumerated input device file is created by `ts_uinput`, you can, among
other thing:

* use the included `tools/ts_uinput_start.sh` script that starts
  `ts_uinput -d -v` and creates the symlink `/dev/input/ts_uinput` for you, or

* if you're using *udev and systemd*, create the following udev rule, for
  example `/etc/udev/rules.d/98-touchscreen.rules`:

      SUBSYSTEM=="input", KERNEL=="event[0-9]*", ATTRS{name}=="ts_uinput", SYMLINK+="input/ts_uinput"

  in case you have to use non-standard paths, create a file containing the
  environment or tslib, like `/etc/ts.env`

      TSLIB_TSDEVICE=/dev/input/ts
      TSLIB_CALIBFILE=/etc/pointercal
      TSLIB_CONFFILE=/etc/ts.conf
      TSLIB_PLUGINDIR=/usr/lib/ts
      TSLIB_FBDEVICE=/dev/fb0

  and create a systemd service file, like `/usr/lib/systemd/system/ts_uinput.service`

      [Unit]
      Description=touchscreen input
      Wants=dev-input-ts_raw.device
      After=dev-input-ts_raw.device

      [Service]
      Type=oneshot
      EnvironmentFile=/etc/ts.env
      ExecStart=/bin/sh -c 'exec /usr/bin/ts_uinput &> /var/log/ts_uinput.log'

      [Install]
      WantedBy=multi-user.target

  and

      #systemctl enable ts_uinput

  will enable it permanently.

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

Parameters:
* `xyswap`

	interchange the X and Y co-ordinates -- no longer used or needed
	if the linear calibration utility `ts_calibrate` is used.

* `pressure_offset`

	offset applied to the pressure value
* `pressure_mul`

	factor to multiply the pressure value with
* `pressure_div`

	value to divide the pressure value by


### module: median
  The median filter reduces noise in the samples' coordinate values. It is
  able to filter undesired single large jumps in the signal. For some
  theory, see [Wikipedia](https://en.wikipedia.org/wiki/Median_filter)

Parameters:
* `depth`

	Number of samples to apply the median filter to


### module: pthres
  Pressure threshold filter. Given a release is always pressure 0 and a
  press is always >= 1, this discards samples below / above the specified
  pressure threshold.

Parameters:
* `pmin`

	Minimum pressure value for a sample to be valid.
* `pmax`

	Maximum pressure value for a sample to be valid.


### module: iir
  Infinite impulse response filter. This is a smoothing filter to remove
  low-level noise. There is a trade-off between noise removal
  (smoothing) and responsiveness. The parameters N and D specify the level of
  smoothing in the form of a fraction (N/D).

  [Wikipedia](https://en.wikipedia.org/wiki/Infinite_impulse_response) has some
  theory.

Parameters:
* `N`

	numerator of the smoothing fraction
* `D`

	denominator of the smoothing fraction


### module: dejitter
  Removes jitter on the X and Y co-ordinates. This is achieved by applying a
  weighted smoothing filter. The latest samples have most weight; earlier
  samples have less weight. This allows to achieve 1:1 input->output rate. See
  [Wikipedia](https://en.wikipedia.org/wiki/Jitter#Mitigation) for some
  theory.

Parameters:
* `delta`

	Squared distance between two samples ((X2-X1)^2 + (Y2-Y1)^2) that
	defines the 'quick motion' threshold. If the pen moves quick, it
	is not feasible to smooth pen motion, besides quick motion is not
	precise anyway; so if quick motion is detected the module just
	discards the backlog and simply copies input to output.


### module: debounce
  Simple debounce mechanism that drops input events for the specified time
  after a touch gesture stopped. [Wikipedia](https://en.wikipedia.org/wiki/Switch#Contact_bounce)
  has more theory.

Parameters:
* `drop_threshold`

	drop events up to this number of milliseconds after the last
	release event.


### module: skip
  Skip nhead samples after press and ntail samples before release. This
  should help if for the device the first or last samples are unreliable.

Parameters:
* `nhead`

	Number of events to drop after pressure
* `ntail`

	Number of events to drop before release


### module:	variance
  Variance filter. Tries to do it's best in order to filter out random noise
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
`ts_calibrate --help` or the man pages for the details. Note that this _only_
rotates the framebuffer output. Rotating the input samples is a different task
that has to be done by the linear filter module (re-calibrating or re-loading
with different parameters).

***


## the libts library
### the libts API
The API is documented in our man pages in the doc directory.
Check out our tests directory for examples how to use it.

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

If a release includes changes like added features, the second number is
incremented and the third is set to zero. If a release includes mostly just
bugfixes, only the third number is incremented.

#### tslib package version
A tslib tarball version number doesn't tell you anything about it's backwards
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

### using libts
If you want to support tslib < 1.2, while still support multitouch and all
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

    #include <tslib.h>

    #define SLOTS 5
    #define SAMPLES 1

    int main(int argc, char **argv)
    {
        struct tsdev *ts;
        char *tsdevice = NULL;
        struct ts_sample_mt **samp_mt = NULL;
        struct input_absinfo slot;
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
				if (samp_mt[j][i].valid != 1)
					continue;

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


### Symbols in Versions
|Name | Introduced|
| --- | --- |
|`TSLIB_VERSION_MT` | 1.10 |
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

### portable `ts_calibrate` and `ts_test_mt` using SDL2

In case you cannot draw to the framebuffer directly, there is an __experimental__
implentation of the necessary graphical tools using SDL2. They are more portable
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

Please note that this list may grow over time. If you rely on
a particular input plugin, you should enable it explicitely. Building __all__
supported modules for your platform should look like so:

* GNU / Linux - all (most importantly `input`)
  - `./configure.ac --enable-cy8mrln-palmpre  --enable-dmc_dus3000`
* Android / Linux - all (most importantly `input`)
  - `./configure.ac --enable-cy8mrln-palmpre  --enable-dmc_dus3000`
* FreeBSD - almost all (most importantly `input`)
  - `./configure.ac --disable-waveshare`
* GNU / Hurd - some, see [hardware support](#touchscreen-hardware-support)
  - `./configure.ac --disable-input --disable-galax --disable-waveshare`
* Haiku - some, see [hardware support](#touchscreen-hardware-support)
  - `./configure.ac --disable-input --disable-galax --disable-touchkit --disable-waveshare`
* Windows - no tslib module for the [Windows touchscreen API](https://msdn.microsoft.com/en-us/library/windows/desktop/dd317323(v=vs.85).aspx) (yet)
  - `./configure.ac --with-sdl2 --disable-ucb1x00 --disable-corgi --disable-collie --disable-h3600 --disable-mk712 --disable-arctic2 --disable-tatung --disable-dmc --disable-input --disable-galax --disable-touchkit --disable-waveshare`

Writing your own plugin is quite easy, in case an existing one doesn't fit.

#### test programs and tools

* GNU / Linux - all
* Android / Linux - all (?)
* FreeBSD - all (?)
* GNU / Hurd - ts_print_mt, ts_print, ts_print_raw, ts_finddev
* Haiku - ts_print_mt, ts_print, ts_print_raw, ts_finddev
* Windows - ts_print.exe, ts_print_raw.exe ts_print_mt.exe ts_test_mt.exe ts_calibrate.exe

#### download binaries?
For GNU/Linux all architectures are _very_ well covered, thanks to Debian or Arch
Linux or others.

If you're lucky, you'll find some unofficial testing builds for Windows or other
platforms [here](https://martinkepplinger.com/tslib/packages/).

Please help porting missing programs!

## touchscreen hardware support
For mostly historical reasons, tslib includes device specific `module_raw` userspace
drivers.
The [ts.conf man page](https://manpages.debian.org/unstable/libts0/ts.conf.5.en.html)
has details on the available `module_raw` drivers; not all of them are listed in the
default `etc/ts.conf` config file. Those are to be considered workarounds and may get
disabled in the default configuration in the future.
If you use one of those, please `./configure --enable-...` it explicitely.

It is strongly recommended to have a real device driver for your system
and use a generic access `module_raw` of tslib. For Linux ([evdev](https://en.wikipedia.org/wiki/Evdev))
this is called `input`.
