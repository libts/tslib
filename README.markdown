
### tslib - a filter stack for accessing and manipulating touchscreen events

![logo](https://raw.githubusercontent.com/kergoth/tslib/tslib_logo_wide.svg)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/11027/badge.svg)](https://scan.coverity.com/projects/tslib)

tslib consists of the library _libts_ and tools that help you _calibrate_ and _use it_ in your environment. There's a [short introductory presentation from 2017](https://fosdem.org/2017/schedule/event/tslib/).

## table of contents
* [setup and configure tslib](#setup-and-configure-tslib)
* [filter modules](#filter-modules)
* [libts - the library](#libts---the-library)
* [tslib development](#tslib-development)


## setup and configure tslib
### install tslib
tslib should be usable on various operating systems, including GNU/Linux, Freebsd or Android Linux. Apart from building the latest tarball release by `./configure`, `make` and `make install`, tslib is available from the following distributors and their package management:
* [Arch Linux](https://www.archlinux.org/packages/?q=tslib) - `pacman -S tslib`
* [Buildroot](https://buildroot.org/) - `BR2_PACKAGE_TSLIB=y`
* (Debian: [Looking for a sponsor](https://mentors.debian.net/package/tslib) to include it)

### set up environment variables
    TSLIB_TSDEVICE          TS device file name.
                            Default (inputapi):     /dev/input/ts
                            /dev/input/touchscreen
                            /dev/input/event0
                            Default (non inputapi): /dev/touchscreen/ucb1x00
    TSLIB_CALIBFILE         Calibration file.
                            Default: ${sysconfdir}/pointercal
    TSLIB_CONFFILE          Config file.
                            Default: ${sysconfdir}/ts.conf
    TSLIB_PLUGINDIR         Plugin directory.
                            Default: ${datadir}/plugins
    TSLIB_CONSOLEDEVICE     Console device.
                            Default: /dev/tty
    TSLIB_FBDEVICE          Framebuffer device.
                            Default: /dev/fb0

* On Debian, `TSLIB_PLUGINDIR` probably is for example `/usr/lib/x86_64-linux-gnu/ts0`.
* Find your `/dev/input/eventX` touchscreen's event file and either
  - Symlink `ln -s /dev/input/eventX /dev/input/ts` or
  - `export TSLIB_TSDEVICE /dev/input/eventX`
* If you are not using `/dev/fb0`, be sure to set `TSLIB_FBDEVICE`

### configure tslib
This is just an example `/etc/ts.conf` file. Touch samples flow from top to bottom. Each line specifies one module and it's parameters. Modules are processed in order. Use _one_ module_raw that accesses your device, followed by any combination of filter modules.

    module_raw input
    module median depth=3
    module dejitter delta=100
    module linear

see the [below](#filter-modules) for available filters and their parameters.

With this configuration file, we end up with the following data flow
through the library:

    driver --> raw read --> median  --> dejitter --> linear --> application
               module       module      module       module

### calibrate the touch screen
Calibration is done by the `linear` plugin, which uses it's own config file `/etc/pointercal`. Don't edit this file manually. It is created by the `ts_calibrate` program:

    # ts_calibrate

The calibration procedure simply requires you to touch the cross on screen, where is appears, as accurate as possible.

### test the filtered input behaviour
You may quickly test the touch behaviour that results from the configured filters, using `ts_test_mt`:

    # ts_test_mt

### use the filtered result in your system
One way to provide your resulting input behaviour to your system, is to use tslib's userspace input driver `ts_uinput`:

    # ts_uinput -d

`-d` make the program return and run as a daemon in the background. Inside of `/dev/input/` there now is a new input event device, which provides your configured input. You can even use a script like `tools/ts_uinput_start.sh` to start the ts_uinput daemon and create a defined `/dev/input/ts_uinput` symlink.

Remember to set your environment and configuration for ts_uinput, just like you did for ts_calibrate or ts_test_mt.

Let's recap the data flow here:

    driver --> raw read --> filter --> (...)   --> ts_uinput  --> evdev read
               module       module     module(s)   application    application


## filter modules

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
  samples have less weight. This allows to achieve 1:1 input->output rate. See
  [Wikipedia](https://en.wikipedia.org/wiki/Jitter#Mitigation) for some general
  theory.

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
  co-ordinates to screen co-ordinates. It applies the corrections as recorded
  and saved by the `ts_calibrate` tool.

#### Parameters:
* `xyswap`

	interchange the X and Y co-ordinates -- no longer used or needed
	if the linear calibration utility `ts_calibrate` is used.

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
  after a touch gesture stopped. [Wikipedia](https://en.wikipedia.org/wiki/Switch#Contact_bounce)
  has more theory.

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
  spikes in the gesture. For some theory, see [Wikipedia](https://en.wikipedia.org/wiki/Median_filter)

Parameters:
* `depth`

	Number of samples to apply the median filter to


***


## libts - the library
### the libts API
Check out the [tests](https://github.com/kergoth/tslib/tree/master/tests) directory for examples how to use it.

`ts_open()`  
`ts_config()`  
`ts_setup()`  
`ts_close()`  
`ts_reconfig()`  
`ts_option()`  
`ts_fd()`  
`ts_load_module()`  
`ts_read()`  
`ts_read_raw()`  
`ts_read_mt()`  
`ts_reat_raw_mt()`  

The API is documented in [our man pages](https://github.com/kergoth/tslib/tree/master/doc). Possibly there will be distributors who provide them online, like [Debian had done for tslib-1.0](https://manpages.debian.org/wheezy/libts-bin/index.html). As soon as there are up-to-date html pages hosted somewhere, we'll link the functions above to it.

### ABI - Application Binary Interface

[Wikipedia](https://en.wikipedia.org/wiki/Application_binary_interface) has background information.

#### Soname versions
Usually, and everytime until now, libts does not break the ABI and your application can continue using libts after upgrading. Specifically this is indicated by the libts library version's major number, which should always stay 0. According to our versioning scheme, the major number is incremented only if we break backwards compatibility. The second or third minor version will increase with releases.

#### tslib package version
Officially, a tslib tarball version number doesn't tell you anything about it's backwards compatibility.

### dependencies

* libc (with libdl if you build it dynamically linked)

### related libraries

* [libevdev](https://www.freedesktop.org/wiki/Software/libevdev/) - access wrapper for [event device](https://en.wikipedia.org/wiki/Evdev) access, uinput too ("Linux API")
* [libinput](https://www.freedesktop.org/wiki/Software/libinput/) - handle input devices for Wayland (uses libevdev)
* [xf86-input-evdev](https://cgit.freedesktop.org/xorg/driver/xf86-input-evdev/) - evdev plugin for X (uses libevdev)

### libts users

* [ts_uinput](https://github.com/kergoth/tslib/blob/master/tools/ts_uinput.c) - userspace event device driver for the tslib-filtered samples
* [xf86-input-tslib](http://public.pengutronix.de/software/xf86-input-tslib/) - **outdated** direct tslib input plugin for X
* [qtslib](https://github.com/qt/qtbase/tree/dev/src/platformsupport/input/tslib) - **outdated** Qt5 qtbase tslib plugin

### using libts
This is a complete example program, similar to `ts_print_mt.c`:

    #include <stdio.h>
    #include <stdlib.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/time.h>
    #include <getopt.h>
    #include <errno.h>
    #include <unistd.h>

    #ifdef __FreeBSD__
    #include <dev/evdev/input.h>
    #else
    #include <linux/input.h>
    #endif

    #include "tslib.h"

    int main(int argc, char **argv)
    {
        struct tsdev *ts;
        char *tsdevice = NULL;
        struct ts_sample_mt **samp_mt = NULL;
        struct input_absinfo slot;
        unsigned short max_slots = 1;
        int ret, i;

        ts = ts_setup(tsdevice, 0);
        if (!ts) {
                perror("ts_setup");
                return errno;
        }

        if (ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
                perror("ioctl EVIOGABS");
                ts_close(ts);
                return errno;
        }

        max_slots = slot.maximum + 1 - slot.minimum;

        samp_mt = malloc(sizeof(struct ts_sample_mt *));
        if (!samp_mt) {
                ts_close(ts);
                return -ENOMEM;
        }
        samp_mt[0] = calloc(max_slots, sizeof(struct ts_sample_mt));
        if (!samp_mt[0]) {
                free(samp_mt);
                ts_close(ts);
                return -ENOMEM;
        }

        while (1) {
                ret = ts_read_mt(ts, samp_mt, max_slots, 1);
                if (ret < 0) {
                        perror("ts_read_mt");
                        ts_close(ts);
                        exit(1);
                }

                if (ret != 1)
                        continue;

                for (i = 0; i < max_slots; i++) {
                        if (samp_mt[0][i].valid != 1)
                                continue;

                        printf("%ld.%06ld: (slot %d) %6d %6d %6d\n",
                               samp_mt[0][i].tv.tv_sec,
                               samp_mt[0][i].tv.tv_usec,
                               samp_mt[0][i].slot,
                               samp_mt[0][i].x,
                               samp_mt[0][i].y,
                               samp_mt[0][i].pressure);
                }
        }

        ts_close(ts);
    }



### Symbols in Versions
|Name | Introduced|
| --- | --- |
|`ts_close` | 1.0 |
|`ts_config` | 1.0 |
|`ts_reconfig` | 1.3 |
|`ts_setup` | 1.4 |
|`ts_error_fn` | 1.0 |
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

## tslib development
### how to contribute
Ideally you fork the project on github, make your changes and create a pull request. You can, however, send your patch to the current maintainer, [Martin Kepplinger](mailto:martink@posteo.de) too. We don't have a mailing list at the moment. Before you send changes, it would be awesome if you checked the following:
* update the NEWS file's changelog if you added a feature
* update or add man pages if applicable
* update the README if applicable
* add a line containing `Fixes #XX` in your git commit message; XX being the github issue's number if you fix an issue
* update the wiki

### module development notes
For those creating tslib modules, it is important to note a couple things with regard to handling of the ability for a user to request more than one ts event at a time. The first thing to note is that the lower layers may send up less events than the user requested, because some events may be filtered out by intermediate layers. Next, your module should send up just as many events as the user requested in nr. If your module is one that consumes events, such as variance, then you loop on the read from the lower layers, and only send the events up when

1. you have the number of events requested by the user, or
2. one of the events from the lower layers was a pen release.

### coding style
We loosely tie to the [Linux coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html)

### release procedure
A release can be done when either
* an issue tagged as "improvement" is implemented, or
* the previous release contains a user visible regression

The procedure looks like this:

1. update the NEWS file with the changelog
* switch to a new release/<version> branch
* update configure.ac versions
  * `AC_INIT` - includes the tslib package version. generally we increment the minor version
  * `LT_CURRENT` - increment **only if there are API changes** (additions / removals / changes)
  * `LT_REVISION` - always increment. should match the minor version of the package version
  * `LT_AGE` - increment **only if `LT_CURRENT` was incremented** and these **API changes are backwards compatible** (should always be the case, so it should match `LT_CURRENT`)
  * `LT_RELEASE` - matches the `AC_INIT` package version

3. update gitignore for autobuilt files
* autobuild and add the files for the tarball to git
* commit "add generated files for X.X release"
* git tag -s X.X
* git push origin release/X.X --tags
* make dist
* shas256sum <tarball> > <tarball>.sha256 for the 3 tarballs
* create a github release off the signed tag
* add the 6 files and the release notes
* publish and inform distributors
* celebrate!

### specifications relevant to tslib

#### evdev interface read by input-raw and offered by ts_uinput
* [Wikipedia evdev](https://en.wikipedia.org/wiki/Evdev)
* [Linux event codes documentation](https://www.kernel.org/doc/Documentation/input/event-codes.txt)
* [Linux event codes definitions](https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/input-event-codes.h)
