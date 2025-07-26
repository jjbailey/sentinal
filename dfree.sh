#!/bin/bash
# dfree.sh
#
# Copyright (c) 2021-2025 jjb
# All rights reserved.
#
# This source code is licensed under the MIT license found
# in the root directory of this source tree.
#/

human_readable()
{
    local bytes=$1
    local units=("B" "K" "M" "G" "T" "P")
    local unit=0
    local size=$bytes

    while ((size >= 1024 && unit < 5)) ; do
        size=$((size / 1024))
        ((unit++))
    done

    if ((unit == 0)) ; then
        echo "${size}${units[$unit]}"
    else
        if command -v bc > /dev/null 2>&1 ; then
            printf "%.1f%s" "$(echo "scale=1 ; $bytes / (1024^$unit)" | bc)" "${units[$unit]}"
        else
            echo "${size}${units[$unit]}"
        fi
    fi
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
    local stat_output

    if stat_output=$(stat -f "$mount_point" 2> /dev/null) ; then
        local total_blocks free_blocks available_blocks block_size

        total_blocks=$(echo "$stat_output" | awk '/Blocks:/ {print $3}')
        free_blocks=$(echo "$stat_output" | awk '/Blocks:/ {print $5}')
        available_blocks=$(echo "$stat_output" | awk '/Blocks:/ {print $7}')
        block_size=$(echo "$stat_output" | awk '/Block size:/ {print $3}')

        local total_bytes=$((total_blocks * block_size))
        local available_bytes=$((available_blocks * block_size))
        local used_bytes=$((total_bytes - available_bytes))

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
    mount | grep -E '^/' | grep -Ev '(/dev/loop|/snap)' | awk '{print $1 " " $3}'
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

max_fs_width=10     # Minimum width
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
