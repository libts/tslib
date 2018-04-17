#!/bin/bash
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2017, Martin Kepplinger <martink@posteo.de>
#
#
# Script to build release-archives with. This requires a checkout from git.
# WARNING: This script is very dangerous! It may delete any untracked files.

set -e

have_version=0
NUMCPUS=$(grep -c '^processor' /proc/cpuinfo)

usage()
{
        echo "Usage: $0 -v version"
}

args=$(getopt -o v:s -- "$@")
if [ $? -ne 0 ] ; then
        usage
        exit 1
fi
eval set -- "$args"
while [ $# -gt 0 ]
do
        case "$1" in
        -v)
                version=$2
                have_version=1
                shift
                ;;
        --)
                shift
                break
                ;;
        *)
                echo "Invalid option: $1"
                usage
                exit 1
                ;;
        esac
        shift
done

# Do we have a desired version number?
if [ "$have_version" -gt 0 ] ; then
       echo "trying to build version $version"
else
       echo "please specify a version"
       usage
       exit 1
fi

# Version number sanity check
if grep ${version} configure.ac
then
       echo "configurations seems ok"
else
       echo "please check your configure.ac"
       exit 1
fi

# Check that we are on master
branch=$(git rev-parse --abbrev-ref HEAD)
echo "we are on branch $branch"

if [ ! "${branch}" = "master" ] ; then
	echo "you don't seem to be on the master branch"
	exit 1
fi


datestamp=`date +"%F"`
sed -i -e "s/^#define LIBTS_DATESTAMP .*/#define LIBTS_DATESTAMP \"$datestamp\"/g" \
 src/ts_version.c

if git diff-index --quiet HEAD --; then
	# no changes
	echo "there are no uncommitted changes (version bump)"
	exit 1
fi
echo "======================================================"
echo "    are you fine with the following version bump?"
echo "======================================================"
git diff
echo "======================================================"
read -p "           Press enter to continue"
echo "======================================================"

./autogen.sh
./configure
make distclean

# Linux all modules build test
./configure --enable-cy8mrln-palmpre \
	--enable-one-wire-ts-input \
	--enable-dmc_dus3000 \
	--enable-galax \
	--enable-arctic2 \
	--enable-corgi \
	--enable-collie \
	--enable-dmc \
	--enable-h3600 \
	--enable-linear-h2200 \
	--enable-mk712 \
	--enable-tatung \
	--enable-ucb1x00

make -j${NUMCPUS}
make clean
make distclean

# Linux SDL2 build test
./configure --with-sdl2

make -j${NUMCPUS}
make clean
make distclean

# static build test
./configure --disable-shared --enable-static \
	--enable-input=static \
	--enable-arctic2=static \
	--enable-collie=static \
	--enable-corgi=static \
	--enable-cy8mrln-palmpre=static \
	--enable-one-wire-ts-input=static \
	--enable-dmc_dus3000=static \
	--enable-dmc=static \
	--enable-galax=static \
	--enable-h3600=static \
	--enable-mk712=static \
	--enable-tatung=static \
	--enable-touchkit=static \
	--enable-ucb1x00=static \
	--enable-waveshare=static \
	--enable-skip=static \
	--enable-pthres=static \
	--enable-debounce=static \
	--enable-median=static \
	--enable-iir=static \
	--enable-invert=static \
	--enable-variance=static \
	--enable-dejitter=static \
	--enable-linear=static \
	--enable-linear-h2200=static \
	--enable-lowpass=static

make -j${NUMCPUS}
make clean
make distclean
./autogen-clean.sh

git clean -d -f

git commit -a -m "tslib ${version}"
git tag -s ${version} -m "tslib ${version}"

./autogen.sh && ./configure && make distcheck
sha256sum tslib-${version}.tar.xz > tslib-${version}.tar.xz.sha256
sha256sum tslib-${version}.tar.gz > tslib-${version}.tar.gz.sha256
sha256sum tslib-${version}.tar.bz2 > tslib-${version}.tar.bz2.sha256

sha512sum tslib-${version}.tar.xz > tslib-${version}.tar.xz.sha512
sha512sum tslib-${version}.tar.gz > tslib-${version}.tar.gz.sha512
sha512sum tslib-${version}.tar.bz2 > tslib-${version}.tar.bz2.sha512

gpg -b -a tslib-${version}.tar.xz
gpg -b -a tslib-${version}.tar.gz
gpg -b -a tslib-${version}.tar.bz2

