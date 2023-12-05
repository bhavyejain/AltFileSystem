# AltFileSystem

AltFileSystem is a fully functional Linux file system written in C using FUSE (Filesystem in Userspace). 

## Overview

AltFileSystem a.k.a Altfs provides functionalities to mount and unmount the filesystem and supports all (almost :P) the capabilities that is required of a Linux filesystem.

## Dependencies

- Libfuse 3

## Installation 

**_NOTE:_**  Currently, the filesystem assumes the disk to be /dev/vdb. We will make this a configurable parameter shortly

### CentOS
1. `dnf -y update`
2. `dnf -y install gcc g++ fuse fuse3-devel autoconf git`
3. `git clone ~/https://github.com/bhavyejain/AltFileSystem.git`
4. `cd ~/AltFileSystem && make clean && make mkaltfs && make filesystem`

## Initializing the file system

1. Create a directory which will be used as the mount point. Example - `mkdir ~/mnt`
2. Run our makefs equivalent using `./AltFileSystem/mkaltfs ~/mnt`
3. Mount the filesystem using `./AltFileSystem/bin/altfs -s ~/mnt`
The filesystem is now ready to use at `~/mnt`.
4. To unmount, run `fusermount -u ~/mnt`

## Debug and logging information
1. To debug and display logs, instead of running step 3 in previous section, run `./AltFileSystem/bin/altfs_debug -f -d -s ~/mnt`
This will run FUSE in the foreground and display logs as each operation is performed in the filesystem.
2. Access and perform the filesystem operations on a separate terminal window.

## Contributors

<a href="https://github.com/bhavyejain/AltFileSystem/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=bhavyejain/AltFileSystem" />
</a>    

<a href="https://github.com/SwathiSBhat/ThePulse/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=SwathiSBhat/ThePulse" />
</a>
