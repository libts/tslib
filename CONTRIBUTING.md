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
provide the output of the `evemu-record` program, selecting your original input
device. `evemu-record` is most likely available as a package in your OS
environment (as evemu-tools).
You can always build it from [source](https://www.freedesktop.org/wiki/Evemu/)
though.

### what to contribute
We collect ideas about improving tslib
[in our issue tracker](https://github.com/libts/tslib/issues). There's a lot
that could be done, for any operating system you might work on.

### how to contribute
Ideally you send your patches to our
[mailing list](http://lists.infradead.org/mailman/listinfo/tslib). Over there
they are discussed and picked up.
You can also fork the project on github, make your changes and create a pull
request.

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
Our library parts are LGPLv2, our programs are GPL licensed. When contributing
or distributing tslib, make sure you comply with the licenses. And please make
sure you have the right to the code you are contributing yourself. There is no
other copyright agreement. You retain the copyright to your code.

### branching model
Simple: our master branch is __not__ to be considered stable! A stable state
will always be tagged (and released as tarballs).

For larger features that aren't in any useful state, there are feature branches
with random names.

Smaller changes can always be done on master and therefore, the master branch
can always be merged into feature branches while they are in development.

When a feature branch's work is stable enough, it is simply merged into master.
We always use `git merge --no-ff`.

### release procedure
A release can be done when either
* the NEWS file fills up or
* the previous release contains a visible regression

The procedure looks like this:

* run coverity (or any static analysis) and fix new discovered issues
* at least `git pull && ./autogen.sh && ./configure && make clean && make` on all supported platforms, all modules configured
* be sure to have a stable build system and your private gpg key set up
* make sure the NEWS file with the changelog and bugfixes is up to date (it should always be).
* update the THANKS file
* increment the correct version numbers in configure.ac and CMakeLists.txt
  * `AC_INIT` - includes the tslib package version X.X. usually we increment the minor version
  * `LT_CURRENT` - increment **only if there are API changes** (additions / removals / changes)
  * `LT_REVISION` - increment if anything changed. but if `LT_CURRENT` was incremented, set to 0!
  * `LT_AGE` - increment **if `LT_CURRENT` was incremented** and these **API changes are backwards compatible** (should always be the case, so it should match `LT_CURRENT`)
* increment the version numbers in the website's file
* do __not__ commit the version change! run `./release -v X.X`
* `git push origin master --tags`
* on Github and Gitlab, edit the release tag:
  * add the release notes from the NEWS file
  * upload all tarball files: 3 times tarball, asc signature and hashsum files
* publish, send an email to the mailing list and if you're in the mood, inform distributors that you know don't automatically get informed.
* celebrate!

note: Do not lose the generated files (tarballs, hashes
and signatures). Always upload them to at least 2 host providers. And do it
immediately after pushing the tag. This also ensures you have time to find a new
one in case one dies. Github is the primary source for distributions.

also note: Really make sure you get the three library version numbers right:
Always collect what's going on in the NEWS file. Build a "changes" and "bugfixes"
section. If there is a significant "changes" section, `LT_CURRENT` and `LT_AGE`
probably will be incremented and `LT_REVISION` set to 0. If there are mostly
bugfixes, only increment `LT_REVISION`. That is what users should be able to
rely on. We don't care how large the numbers may get. For the automatically
generated libts.so version name, the major version usually becomes "current - age",
so as long as they stay the same, it stays 0. The tarball version number
set in `AC_INIT` doesn't matter that much.

### specifications relevant to tslib

#### evdev interface read by input-raw and offered by ts_uinput
* [Wikipedia evdev](https://en.wikipedia.org/wiki/Evdev)
* [Linux event codes documentation](https://www.kernel.org/doc/Documentation/input/event-codes.txt)
* [Linux event codes definitions](https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/input-event-codes.h)

### project organisation for new maintainers

The tslib project currently consists of
* the [libts/tslib](https://github.com/libts/tslib) github repository. This
is the main upstream repository users download from. The
maintainer of course must have access to it, at least for committing and
creating releases. When becoming a new maintainer, ask one of the "libts"
organisation's owners to give you access, or become an owner too.
* the [tslib.org](http://tslib.org) project page. It is currently registered at
[easyname](https://www.easyname.com) and is maintained in the website directory.
This project site must include links to *all parts* of the tslib project.
That's this list right here. When becoming a new maintainer, preferably make
an account at easyname and the old maintainer will transfer the domain to you.
Currently that is Martin Kepplinger. The domain is about 14 Euros per year.
* the [tslib/tslib](https://gitlab.com/tslib/tslib) gitlab repository. This
is currently simply a backup that pulls in every changes from github once a
day. We want to have it in case github is down. Although it'd be easy to
move development over to gitlab, it's not easy to have distributors and users
follow. So this is currently a backup only. Martin Kepplinger currently
holds the "tslib" gitlab user-account and will hand it over to the new
maintainer.
* the [mainling list](http://lists.infradead.org/mailman/listinfo/tslib) at
infradead.org. It's pretty self-explanatory and shouldn't look unfamiliar to
you.
