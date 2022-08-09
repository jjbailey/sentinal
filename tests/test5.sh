#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:

# script to test test5.ini
# pcrestr   = test5-

LOGDIR=/opt/sentinal/tests

# In different windows:

# /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test5.ini
# /opt/sentinal/tests/test5.sh

for i in {1..20} ; do
    LOGFILE="test5-$(date +%F_%H-%M-%S).log"
    echo "LOGFILE=$LOGFILE"
    dd if=/dev/urandom of=$LOGDIR/$LOGFILE bs=256K count=1
    sleep 5
done

exit 0
