## tslib development
### reporting bugs
You are free to report bugs using Github issues or the
[mailing list](http://lists.infradead.org/mailman/listinfo/tslib). For general
questions, discussions or thoughts, we'd prefer the list though.

When describing a problem, please always tell us the following:
* your architecture and operating system
* the version of tslib you are using
* the ts.conf configuration that causes the problem for you

If tslib can't use your touchscreen device, either by not starting at all or
by behaving strangely, at least on Linux and BSD it is always helpful if you
provide the output of the `evtest` program, selecting your original input
device. `evtest` is most likely available as a package in your OS environment.
You can always build it from [upstream](https://cgit.freedesktop.org/evtest)
though.

### what to contribute
We collect ideas about improving tslib
[in our issue tracker](https://github.com/kergoth/tslib/issues). There's a lot
that could be done, for any operating system you might work on.

### how to contribute
Ideally you send your patches to our
[mailing list](http://lists.infradead.org/mailman/listinfo/tslib). OVer there
they are discussed and picked up.
You can also fork the project on github, make your changes and create a pull
request. Before you send changes, it would be awesome if you checked the
following:
* update the NEWS file's changelog if you added a feature
* update or add man pages if applicable
* update the README if applicable
* possibly add a line containing `Fixes #XX` in your git commit message; XX being the github issue's number

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

In case of a filter module, add it to ts_verify and make sure test runs for
read_mt at least all pass.

### coding style
We loosely tie to the [Linux coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html)

### license and copyright
Our library parts are LGPLv2, Our programs are GPL licensed. When contributing
or distributing tslib, make sure you comply with the licenses. And please make
sure you have the right to the code you are contributing yourself. There is no
other copyright aggreement. You retain your copyright to your code.

### branching model
The short version is: there really is none. But what is important is that
our master branch is __not__ to be considered stable! A stable state will always
be tagged (and released as tarballs).

And please simply ignore the release/X.X branches! They are old.

For larger features that aren't in any useful state, there are feature branches
with random names.

Smaller changes can always be done on master and therefore, the master branch
can always be merged into feature branches, while they are in development.

When a feature branch's work is stable enough, it is simply merged into master.
We always use `git merge -no-ff`.

### release procedure
A release can be done when either
* the NEWS file fills up or
* the previous release contains a visible regression

The procedure looks like this:

* run coverity (or any static analysis) and fix new discovered issues
* at least `./autogen.sh && ./configure && make` on all supported operating systems
* be sure to have a stable build system and your private gpg key set up
* update the NEWS file with the changelog and bugfixes
* update the THANKS file
* increment the correct version numbers in configure.ac
  * `AC_INIT` - includes the tslib package version X.X. generally we increment the minor version
  * `LT_CURRENT` - increment **only if there are API changes** (additions / removals / changes)
  * `LT_REVISION` - increment if anything changed. but if `LT_CURRENT` was incremented, set to 0!
  * `LT_AGE` - increment **only if `LT_CURRENT` was incremented** and these **API changes are backwards compatible** (should always be the case, so it should match `LT_CURRENT`)

* do __not__ commit the version change! run `./release -v X.X`
* `git push origin master --tags`
* create a github release off the signed tag by adding
  * release notes from the NEWS file
  * 3 times: tarball, asc signature and sha256sum files
* publish and inform distributors
* celebrate!
* when the backup [gitlab repository](https://gitlab.com/tslib/tslib) pulled the release tag in (after about one day), add the tarballs and release notes there.

sidenote: up until 1.7 we kept the habit of adding generated files to a release
branch that is never used again. Those branches exist. We can't do anything
about it. We won't create new ones anymore. Ignore them.

### specifications relevant to tslib

#### evdev interface read by input-raw and offered by ts_uinput
* [Wikipedia evdev](https://en.wikipedia.org/wiki/Evdev)
* [Linux event codes documentation](https://www.kernel.org/doc/Documentation/input/event-codes.txt)
* [Linux event codes definitions](https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/input-event-codes.h)

### project organisation for new maintainers

The tslib project currently consists of
* the [kergoth/tslib](https://github.com/kergoth/tslib) github repository. This
is the main upstream repository users download from. The
maintainer of course must have access to it, at least for committing and
creating releases. When becoming a new maintainer, ask Christopher Larson, who
holds the "kergoth" github account, to give you access.
* the [tslib.org](http://tslib.org) project page. It is currently registered at
[easyname](https://www.easyname.com) and is maintained in the website directory.
This project site must include links to *all parts* of the tslib project.
That's this list right here. When becoming a new maintainer, preferrably make
an account at easyname and the old maintainer will transfer the domain to you.
Currently that is Martin Kepplinger. The domain is about 14 Euros per year.
* the [tslib/tslib](https://gitlab.com/tslib/tslib) gitlab repository. This
is currently simply a backup that pulls in every changes from github once a
day. We want to have it in case github is down. Although it'd be easy to
move development over to gitlab, it's not easy to have distributors and users
follow you. So this is currently a backup only. Martin Kepplinger currently
holds the "tslib" gitlab user-account and will hand it over to the new
maintainer.
* the [mainling list](http://lists.infradead.org/mailman/listinfo/tslib) at
infradead.org. It's pretty self-explanatory and shouldn't look unfamiliar to
you.
