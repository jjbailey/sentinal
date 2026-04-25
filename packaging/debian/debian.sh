#!/bin/bash
# debian.sh
# vim: set tabstop=4 shiftwidth=4 expandtab:

# creates a control file and creates a deb from the current installation
# to run this:
# make install
# make deb

# Copyright (c) 2021-2026 jjb
# All rights reserved.
#
# This source code is licensed under the MIT license found
# in the root directory of this source tree.

unset CDPATH
set -euo pipefail

die()
{
    echo "$*" 1>&2
    exit 1
}

require_commands()
{
    local cmd

    for cmd in "$@" ; do
        command -v "$cmd" > /dev/null 2>&1 || die "missing command: $cmd"
    done
}

[ -x /opt/sentinal/bin/sentinal ] || {
    die "sentinal is not installed"
}

require_commands awk cp dpkg dpkg-deb dpkg-architecture find

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd -P)
cd "$SCRIPT_DIR" || exit 1

VERSION=$(/opt/sentinal/bin/sentinal -V 2>&1 | awk 'NF { print $NF; exit }')
[ -n "$VERSION" ] || die "cannot determine sentinal version"
printf '%s\n' "$VERSION" | awk '
    /^[0-9][0-9A-Za-z.+:~-]*$/ { ok=1 }
    END { exit ok ? 0 : 1 }
' || die "invalid debian version: $VERSION"

PACKAGE=${VERSION}-1
BUILDIR=sentinal_${PACKAGE}
DEBARCH=$(dpkg-architecture -qDEB_HOST_ARCH)
CWD=$PWD

mkdir -p "$BUILDIR/DEBIAN"
rm -rf -- "$BUILDIR/opt"
mkdir -p "$BUILDIR"
cp -pr --parents /opt/sentinal "$BUILDIR"
find "$BUILDIR" -name '*~' -delete
find "$BUILDIR" -type p -delete

cat << EOF > "$BUILDIR/DEBIAN/control"
Package: sentinal
Version: $PACKAGE
License: MIT
Architecture: $DEBARCH
Maintainer: JJB <itguy92075@gmail.com>
Vcs-Git: https://github.com/jjbailey/sentinal.git
Section: base
Priority: optional
Depends: libpcre2-8-0
Description: Software for Logfile and Inode Management
 Copyright (c) 2021-2026 jjb
EOF

dpkg-deb --build -Z gzip "$BUILDIR"

DEBFILE="$CWD/$BUILDIR.deb"
[ -f "$DEBFILE" ] || die "deb build finished but package was not found: $DEBFILE"

dpkg -c "$DEBFILE"
ls -o "$DEBFILE"

exit 0
