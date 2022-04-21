#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:

# script to test test7.ini
# pcrestr  = file\d

LOGDIR=/var/tmp/tmp

# In different windows:

# mkdir -p $LOGDIR
# /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test7.ini
# /opt/sentinal/tests/test7.sh

mkdir -p $LOGDIR

for i in {1..9} ; do
    LOGFILE=file$i
    echo "LOGFILE=$LOGFILE"
    dd if=/dev/zero of=$LOGDIR/$LOGFILE bs=1M count=2
    sleep 10
done

exit 0
