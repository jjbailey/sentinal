#!/bin/bash
# redhat.sh
# vim: set tabstop=4 shiftwidth=4 expandtab:

# creates a spec file and creates an rpm from the current installation
# to run this:
# make install
# make rpm

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

require_commands awk cp find rpm rpmbuild rpmdev-setuptree sed sort tar

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd -P)
cd "$SCRIPT_DIR" || exit 1

VERSION=$(/opt/sentinal/bin/sentinal -V 2>&1 | awk 'NF { print $NF; exit }')
[ -n "$VERSION" ] || die "cannot determine sentinal version"

VERSION_RPM=${VERSION%%-*}
RELEASE_RPM=1
if [ "$VERSION_RPM" != "$VERSION" ] ; then
    RELEASE_RPM=${VERSION#"$VERSION_RPM"-}
fi
RELEASE_RPM=$(echo "$RELEASE_RPM" | sed 's/[^A-Za-z0-9._]/_/g')
[ -n "$VERSION_RPM" ] || die "cannot determine rpm version"
[ -n "$RELEASE_RPM" ] || die "cannot determine rpm release"

BUILDIR=sentinal-$VERSION
TARBALL=sentinal-$VERSION.tar.gz
RPMARCH=$(rpm --eval '%{_arch}')
SPECFILE=sentinal.spec
RPMFILE=sentinal-$VERSION_RPM-$RELEASE_RPM.$RPMARCH.rpm

# rpmdev-setuptree?
export HOME="$PWD"

mkdir -p "$HOME/rpmbuild" || exit 1
(cd "$HOME/rpmbuild" && rpmdev-setuptree)
rm -rf -- "$BUILDIR"
mkdir -p "$BUILDIR"
cp -pr --parents /opt/sentinal "$BUILDIR"
find "$BUILDIR" -name '*~' -delete
find "$BUILDIR" -type p -delete
tar czf "$HOME/rpmbuild/SOURCES/$TARBALL" "$BUILDIR"

(

    cat << EOF
Name:           sentinal
Version:        $VERSION_RPM
Release:        $RELEASE_RPM
Summary:        Software for Logfile and Inode Management

License:        MIT
URL:            https://github.com/jjbailey/sentinal.git
Source0:        $TARBALL

Requires:       pcre2

Packager:       JJB <itguy92075@gmail.com>
Group:          System Management

%description
Software for Logfile and Inode Management
Copyright (c) 2021-2026 jjb

%prep
%setup -q -n $BUILDIR

%install
EOF

    cd "$BUILDIR" || exit 1

    while IFS= read -r -d '' d ; do
        printf 'install -m 755 -d "$RPM_BUILD_ROOT/%s"\n' "$d"
    done < <(find opt/sentinal -type d -print0 | sort -z)

    while IFS= read -r -d '' f ; do
        [[ $f == */bin/* ]] && mode=755 || mode=644
        printf 'install -m %s "%s" "$RPM_BUILD_ROOT/%s"\n' "$mode" "$f" "$f"
    done < <(find opt/sentinal -type f -print0 | sort -z)

    echo 'chown -R root:root $RPM_BUILD_ROOT/opt/sentinal'

    echo
    echo "%files"
    echo /opt/sentinal

) > "$HOME/rpmbuild/SPECS/$SPECFILE"

rpmbuild -bb "$HOME/rpmbuild/SPECS/$SPECFILE"

RPMPATH="$HOME/rpmbuild/RPMS/$RPMARCH/$RPMFILE"
[ -f "$RPMPATH" ] || die "rpm build finished but package was not found: $RPMPATH"

# Verify the package archive itself. `rpm -Vp` compares packaged files to the
# current filesystem and can fail after normal rpmbuild post-processing.
rpm -K "$RPMPATH"
ls -o "$RPMPATH"

exit 0
