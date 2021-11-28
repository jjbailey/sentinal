#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:
# redhat.sh

# creates a spec file and creates an rpm from the current installation
# to run this:
# make install
# make rpm

# Copyright (c) 2021 jjb
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
read MAJOR MINOR <<< $(echo $VERSION | sed 's/\./ /')
BUILDIR=sentinal-$MAJOR
RPMFILE=sentinal-$MAJOR-$MINOR.x86_64.rpm

rm -fr rpmbuild $BUILDIR
mkdir -p rpmbuild $BUILDIR || exit 1
(cd rpmbuild && rpmdev-setuptree)
cp -pr --parents /opt/sentinal $BUILDIR
find $BUILDIR -name '*~' -delete
find $BUILDIR -type p -delete
tar czvf rpmbuild/SOURCES/sentinal-$MAJOR.$MINOR.tar.gz $BUILDIR

(

    cat << EOF
Name:           sentinal
Version:        $MAJOR
Release:        $MINOR
Summary:        Software for logfile and inode management

License:        MIT
URL:            https://github.com/jjbailey/sentinal.git
Source0:        %{name}-%{version}.%{release}.tar.gz

Requires:       pcre2

Packager:       JJB <itguy92075@gmail.com>
Group:          System Management

%description
Software for logfile and inode management
Copyright (c) 2021 jjb

%prep
%setup -q

%install
EOF

    cd $BUILDIR || exit 1

    for d in $(find opt/* -type d | sort); do
        echo "install -m 755 -d \$RPM_BUILD_ROOT/$d"
    done

    for f in $(find opt/* -type f | sort); do
        [[ $f == */bin/* ]] && mode=755 || mode=644
        echo "install -m $mode $f \$RPM_BUILD_ROOT/$f"
    done

    echo 'chown -R root:root $RPM_BUILD_ROOT/opt/sentinal'

    echo
    echo "%files"
    find /opt/sentinal/* -type d | sort

) > rpmbuild/SPECS/sentinal.spec

rpmbuild -bb rpmbuild/SPECS/sentinal.spec
[ -f rpmbuild/RPMS/x86_64/$RPMFILE ] && rpm -Vp rpmbuild/RPMS/x86_64/$RPMFILE
exit 0
