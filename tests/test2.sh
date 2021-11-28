#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:

# script to test test2.ini
# pcrestr  = inode-

LOGDIR=/opt/sentinal/tests

# In different windows:

# /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test2.ini
# /opt/sentinal/tests/test2.sh

# this script creates files much faster than sentinal removes them

for i in {1..5} ; do
    for j in {1..10000} ; do
        INDEX=$(date +%H/%M/%S)
        LOGFILE="inode-$(printf '%05d' $RANDOM)"
        mkdir -p $LOGDIR/$INDEX
        echo $LOGDIR/$INDEX/$LOGFILE
        touch $LOGDIR/$INDEX/$LOGFILE
    done

    sleep 10
done

exit 0
