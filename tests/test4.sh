#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:

# script to test test4.ini
# pcrestr  = test4-

LOGDIR=/opt/sentinal/tests

# In different windows:

# /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test4.ini
# /opt/sentinal/tests/test4.sh

LOGFILE=test4.fifo
echo $LOGFILE
dd if=/dev/urandom of=$LOGDIR/$LOGFILE bs=1M count=10240
exit 0
