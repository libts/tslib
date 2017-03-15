## tslib development
### what to contribute
We collect all non-trivial thoughts about improving tslib, as well as bugreports
[in our issue tracker](https://github.com/kergoth/tslib/issues). There's a lot
that could be done, for any operating system you might work on.

### how to contribute
Ideally you fork the project on github, make your changes and create a pull
request. You can, however, send your patch to the current maintainer,
[Martin Kepplinger](mailto:martink@posteo.de) too. We don't have a mailing list
at the moment. Before you send changes, it would be awesome if you checked the
following:
* update the NEWS file's changelog if you added a feature
* update or add man pages if applicable
* update the README if applicable
* add a line containing `Fixes #XX` in your git commit message; XX being the github issue's number if you fix an issue
* update the wiki

### module development notes
For those creating tslib modules, it is important to note a couple things with
regard to handling of the ability for a user to request more than one ts event
at a time. The first thing to note is that the lower layers may send up less
events than the user requested, because some events may be filtered out by
intermediate layers. Next, your module should send up just as many events as
the user requested in nr. If your module is one that consumes events, such as
variance, then you loop on the read from the lower layers, and only send the
events up when

1. you have the number of events requested by the user, or
2. one of the events from the lower layers was a pen release.

In your implementation of `read_mt()` you must first call the next module's
`read_mt()` and in case of a negative return value, return that error code.
Otherwise of course return the actual number of samples you provide after your
processing.

### coding style
We loosely tie to the [Linux coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html)

### license and copyright
Our library parts are LGPLv2, Our programs are GPL licensed. When contributing
or distributing tslib, make sure you comply with the licenses. And please make
sure you have the right to the code you are contributing yourself. There is no
other copyright aggreement. You retain your copyright to your code.

### release procedure
A release can be done when either
* an issue tagged as "improvement" is implemented, or
* the previous release contains a user visible regression

The procedure looks like this:

1. run coverity (or any static analysis) and fix new discovered issues
* be sure to have a stable build system and your private gpg key set up
* update the NEWS file with the changelog and bugfixes
* update the THANKS file
* update configure.ac libts library versions
  * `AC_INIT` - includes the tslib package version X.X. generally we increment the minor version
  * `LT_CURRENT` - increment **only if there are API changes** (additions / removals / changes)
  * `LT_REVISION` - increment if anything changed. but if `LT_CURRENT` was incremented, set to 0!
  * `LT_AGE` - increment **only if `LT_CURRENT` was incremented** and these **API changes are backwards compatible** (should always be the case, so it should match `LT_CURRENT`)

6. create a new release/X.X branch remotely and switch to it locally
* `./release -v X.X`
* `git push origin release/X.X --tags`
* create a github release off the signed tag by adding
  * release notes from the NEWS file
  * 3 times: tarball, asc signature and sha256sum
10. publish and inform distributors
* celebrate!

### specifications relevant to tslib

#### evdev interface read by input-raw and offered by ts_uinput
* [Wikipedia evdev](https://en.wikipedia.org/wiki/Evdev)
* [Linux event codes documentation](https://www.kernel.org/doc/Documentation/input/event-codes.txt)
* [Linux event codes definitions](https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/input-event-codes.h)

### project organisation for new maintainers

TODO
