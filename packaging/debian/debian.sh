#!/bin/bash
# debian.sh
# vim: set tabstop=4 shiftwidth=4 expandtab:

# creates a control file and creates a deb from the current installation
# to run this:
# make install
# make deb

# Copyright (c) 2021, 2022 jjb
# All rights reserved.
#
# This source code is licensed under the MIT license found
# in the root directory of this source tree.

unset CDPATH

[ -x /opt/sentinal/bin/sentinal ] || {
    echo "sentinal is not installed"
    exit 1
}

SCRIPT_PATH=${0%/*}
if [ -d "$SCRIPT_PATH" ] ; then
    cd $(realpath $SCRIPT_PATH) || exit 1
fi

VERSION=$(/opt/sentinal/bin/sentinal -V 2>&1 | awk '{ print $3 }')
PACKAGE=${VERSION}-1
BUILDIR=sentinal_${PACKAGE}

mkdir -p $BUILDIR/DEBIAN
CWD=$PWD
rm -fr $BUILDIR/opt
cp -pr --parents /opt/sentinal $BUILDIR
find $BUILDIR -name '*~' -delete
find $BUILDIR -type p -delete

cat << EOF > $BUILDIR/DEBIAN/control
Package: sentinal
Version: $PACKAGE
License: MIT
Architecture: amd64
Maintainer: JJB <itguy92075@gmail.com>
Vcs-Git: https://github.com/jjbailey/sentinal.git
Section: base
Priority: optional
Depends: libpcre2-8-0
Description: Software for Logfile and Inode Management
 Copyright (c) 2021, 2022 jjb
EOF

dpkg-deb --build $BUILDIR

[ -f $BUILDIR.deb ] && {
    dpkg -c $CWD/$BUILDIR.deb
    ls -o $CWD/$BUILDIR.deb
}

exit 0
