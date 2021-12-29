#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:

# script to test test6.ini

LOGDIR=/opt/sentinal/tests
LOGFILE="test6.log"

# In different windows:

# /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test6.ini
# /opt/sentinal/tests/test6.sh

# this test intentionally runs slowly.

for i in {1..60} ; do
    for dir in /etc /usr /var ; do
        find $dir >> $LOGDIR/$LOGFILE 2> /dev/null
        sleep $((RANDOM % 9 + 1))
    done

    sleep $((RANDOM % 10 + 60))
done

exit 0
