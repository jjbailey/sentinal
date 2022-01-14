#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:

# script to test test1.ini
# pcrestr  = console-

LOGDIR=/opt/sentinal/tests

# In different windows:

# /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test1.ini
# /opt/sentinal/tests/test1.sh

for i in {1..100} ; do
    LOGFILE="console-$(date +%Y-%m-%d_%H-%M-%S).log"
    echo "LOGFILE=$LOGFILE"
    timeout 5 dd if=/dev/urandom of=$LOGDIR/$LOGFILE bs=256K
    sleep 1
done

exit 0
