#!/bin/bash

# Copyright (C) 2017, Martin Kepplinger <martink@posteo.de>

# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution.

# Script to build release-archives with. This requires a checkout from git.

# WARNING: This script is very dangerous! It may delete any untracked files.

have_version=0

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

./autogen.sh && ./configure && make distcheck
./configure --disable-dependency-tracking && make distclean && ./autogen-clean.sh
git clean -d -f

git commit -a -m "tslib ${version}"
git tag -s ${version} -m "tslib ${version}"

./autogen.sh && ./configure && make distcheck
sha256sum tslib-${version}.tar.xz > tslib-${version}.tar.xz.sha256
sha256sum tslib-${version}.tar.gz > tslib-${version}.tar.gz.sha256
sha256sum tslib-${version}.tar.bz2 > tslib-${version}.tar.bz2.sha256

gpg -b -a tslib-${version}.tar.xz
gpg -b -a tslib-${version}.tar.gz
gpg -b -a tslib-${version}.tar.bz2

