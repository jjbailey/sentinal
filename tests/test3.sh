#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:

# script to test test3.ini
# pcrestr  = test3-

LOGDIR=/opt/sentinal/tests

# In different windows:

# /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test3.ini
# /opt/sentinal/tests/test3.sh

for i in {1..10} ; do
    LOGFILE=test3.fifo
    echo "LOGFILE=$LOGFILE"
    dd if=/dev/zero of=$LOGDIR/$LOGFILE bs=1M count=10240
    sleep 1
done

exit 0
