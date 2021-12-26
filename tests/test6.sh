#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:

# script to test test6.ini

LOGDIR=/opt/sentinal/tests
LOGFILE="test6.log"

# In different windows:

# /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test6.ini
# /opt/sentinal/tests/test6.sh

dd if=/dev/urandom of=$LOGDIR/$LOGFILE bs=1M count=500
exit 0
