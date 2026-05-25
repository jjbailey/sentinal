#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:

# Example script for watching sentinal threads.
# Usage: watch [options] watch-sentinal.sh

PATH=/usr/sbin:/usr/bin:/bin

trap 'rm -f /tmp/sentinal_cmdline_*' INT TERM

PIDS=$(pidof sentinal)

for pid in $PIDS; do
    cache="/tmp/sentinal_cmdline_${pid}"
    [ -f "$cache" ] || tr '\0' ' ' < /proc/"$pid"/cmdline | sed 's/ $//' > "$cache"
    echo "$(<"$cache")"
    top -Hbn1 -p "$pid" | sed '1,6d'
    echo
done
