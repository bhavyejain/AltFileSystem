#!/bin/bash

# Check if exactly two arguments are given
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <device_path> <size_in_gb>"
    exit 1
fi

# Assign arguments to variables for clarity
DEVICE_PATH="$1"
SIZE_IN_GB="$2"

# Check if the second argument is an integer
if ! [[ "$SIZE_IN_GB" =~ ^[0-9]+$ ]]; then
    echo "The second argument must be an integer representing the size in GB."
    exit 1
fi

mkdir -p ./bin
gcc -o ./bin/mkaltfs ./src/mkfs.c -g -Og -I./header -Wall -std=gnu11 `pkg-config fuse3 --cflags --libs` -DDISK_MEMORY -DDEVICE_NAME=\""$DEVICE_PATH"\" -DFS_GB=$SIZE_IN_GB

if [ $? -eq 0 ]; then
    ./bin/mkaltfs
    rm -rf ./bin
else
    echo "Building mkaltfs failed"
    exit 1
fi