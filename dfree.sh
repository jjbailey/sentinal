#!/bin/bash
# dfree.sh
#
# Copyright (c) 2021-2025 jjb
# All rights reserved.
#
# This source code is licensed under the MIT license found
# in the root directory of this source tree.
#

human_readable()
{
    local bytes="$1"

    # Defensive: treat empty/non-numeric as zero
    if [[ -z $bytes || $bytes != [0-9]* ]] ; then
        echo "0B"
        return
    fi

    # Use awk (floating arithmetic) to avoid bash 64-bit overflow issues.
    awk -v b="$bytes" 'BEGIN {
        split("B K M G T P E", u, " ") ;
        if (b == 0) { print "0B" ; exit }
        unit = 1
        val = b + 0
        while (val >= 1024 && unit < length(u)) {
            val = val / 1024
            unit++
        }
        # one decimal like your original function
        printf "%.1f%s\n", val, u[unit]
    }'
}

calc_percentage()
{
    local used=$1
    local total=$2

    if ((total == 0)) ; then
        echo "0.00%"
    else
        if command -v bc > /dev/null 2>&1 ; then
            printf "%.2f%%" "$(echo "scale=2 ; $used * 100 / $total" | bc)"
        else
            printf "%.2f%%" "$(awk "BEGIN {printf \"%.2f\", $used * 100 / $total}")"
        fi
    fi
}

get_fs_info()
{
    local mount_point=$1
    local df_output
    if df_output=$(df -B1 "$mount_point" 2> /dev/null | awk 'NR==2') ; then
        local total_bytes used_bytes available_bytes
        read -r _ total_bytes used_bytes available_bytes _ _ <<< "$df_output"
        echo "$total_bytes $used_bytes $available_bytes"
    else
        echo ""
    fi
}

show_usage()
{
    echo "Usage: $0 [mount_point1] [mount_point2] ..."
    echo "       $0                 # Show all mounted filesystems"
    echo
    echo "Examples:"
    echo "  $0                      # Show all filesystems"
    echo "  $0 /home                # Show only /home filesystem"
    echo "  $0 / /home /var         # Show root, home, and var filesystems"
}

get_specific_mounts()
{
    local mount_points=("$@")
    local mount_info=""

    for mount_point in "${mount_points[@]}" ; do
        if [[ ! -d $mount_point ]] ; then
            echo "Warning: Mount point '$mount_point' does not exist or is not accessible" >&2
            continue
        fi

        local device=$(mount | grep " on $mount_point " | awk '{print $1}')
        if [[ -n $device ]] ; then
            mount_info+="$device $mount_point"$'\n'
        else
            echo "Warning: '$mount_point' is not a mounted filesystem" >&2
        fi
    done

    echo "$mount_info"
}

get_all_mounts()
{
    findmnt -rn -o SOURCE,TARGET -t ext4,xfs,nfs,nfs4,cifs |
        grep -Ev '(/dev/loop|/var/snap|/snap)'
}

# Main script logic
if [[ $# -eq 0 ]] ; then
    mount_info=$(get_all_mounts)
else
    # Check for help flags
    for arg in "$@" ; do
        if [[ $arg == "-h" || $arg == "--help" || $arg == "-?" ]] ; then
            show_usage
            exit 0
        fi
    done

    mount_info=$(get_specific_mounts "$@")
fi

if [[ -z $mount_info ]] ; then
    echo "No filesystems found to display."
    exit 0
fi

max_fs_width=10
while IFS=' ' read -r device mount_point ; do
    [[ ! -d $mount_point ]] && continue

    fs_info=$(get_fs_info "$mount_point")
    [[ -z $fs_info ]] && continue

    device_length=${#device}
    if ((device_length > max_fs_width)) ; then
        max_fs_width=$device_length
    fi
done <<< "$mount_info"

((max_fs_width++))

printf "%-${max_fs_width}s %10s %10s %10s %7s %s\n" "Filesystem" "Size" "Used" "Avail" "Use%" "Mounted on"

while IFS=' ' read -r device mount_point ; do
    [[ ! -d $mount_point ]] && continue

    fs_info=$(get_fs_info "$mount_point")
    [[ -z $fs_info ]] && continue

    read -r total_bytes used_bytes available_bytes <<< "$fs_info"

    total_human=$(human_readable $total_bytes)
    used_human=$(human_readable $used_bytes)
    available_human=$(human_readable $available_bytes)
    use_percent=$(calc_percentage $used_bytes $total_bytes)

    printf "%-${max_fs_width}s %10s %10s %10s %7s %s\n" \
        "$device" \
        "$total_human" \
        "$used_human" \
        "$available_human" \
        "$use_percent" \
        "$mount_point"
done <<< "$mount_info"

# vim: set tabstop=4 shiftwidth=4 expandtab:
