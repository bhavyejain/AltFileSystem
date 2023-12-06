# AltFileSystem

AltFileSystem is a functional Linux file system written in C wrapped in FUSE (Filesystem in Userspace). 

## Overview

AltFileSystem a.k.a Altfs provides functionalities to mount and unmount the filesystem and supports all (almost :P) the capabilities that is required of a Linux filesystem.

## Dependencies

- Libfuse 3

## Installation 

**_NOTE:_**  Currently, the filesystem assumes the disk to be /dev/vdb. We will make this a configurable parameter shortly. The device name and filesystem size are currently configurable through the `header/disk_layer.h` file.

### Building AltFS on CentOS 9
1. `dnf -y update`
2. `dnf -y install gcc g++ fuse fuse3-devel autoconf git`
3. `git clone ~/https://github.com/bhavyejain/AltFileSystem.git`
4. `cd ~/AltFileSystem && make clean && make mkaltfs && make filesystem`

## Initializing the file system

1. Create a directory which will be used as the mount point. Example - `mkdir ~/mnt`
2. Run our makefs equivalent using `./AltFileSystem/mkaltfs`
3. Mount the filesystem using `./AltFileSystem/bin/altfs -s ~/mnt`
The filesystem is now ready to use at `~/mnt`.
4. To unmount, run `fusermount -u ~/mnt`

## Debug and logging information
1. To debug AltFS, make the filesystem in debug mode: `make filesystem_debug`
2. To debug and display logs, instead of running step 3 in previous section, run `./AltFileSystem/bin/altfs_debug -d -s ~/mnt`
This will run FUSE in the foreground and display logs as each operation is performed in the filesystem.
3. If you wish to view more selective logging, run `./AltFileSystem/bin/altfs_debug -f -s ~/mnt`.
4. Access and perform the filesystem operations on a separate terminal window.

## Development
Development is aided by a suite of unit and e2e tests. For initial development cycle, in-memory unit tests provide a fast and no-setup option. For on-disk validation, e2e tests provide high-level test cases.

### Unit Tests
The unit tests are numbered in an increasing order corresponding to their position in the stack. They are intended to be run in the same order while testing the filesystem in its entirity. A failing lower test almost nullifies any guarantees by a higher numbered test.
1. The `run_tests.sh` provides for automatically building and running all tests in order, or a selected test. To run all tests, use: `./run_tests.sh`. To run a specific test, add the name of the test as an argument: `./run_tests.sh 01_disk_layer`. All tests are run in-memory.
2. `make unit_tests` builds all the unit tests in 'in-memory' mode (no data is written on disk).
3. Each test can be made individually as `make <testname-without-extension>`, e.g.: `make 01_disk_layer`.
4. The interface layer test can be made to run on the disk as well: `make test_interface_layer_disk`. It is a good idea to run this test on disk at the end of making any changes. Tests can be toggled by commenting out the function call in main().

### E2E Tests
All e2e tests are intended to be run on disk. AltFS must be mounted beforehand. Clone the repository inside the mountpoint (or copy the e2e test folder). Make the test(s) and run them inside the mount point.
1. `make all` will build all tests.
2. Each individual test can be built in the same was as described for unit tests.
3. These tests need not be run in order.

## Contributors

<a href="https://github.com/bhavyejain/AltFileSystem/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=bhavyejain/AltFileSystem" />
</a>    

<a href="https://github.com/SwathiSBhat/ThePulse/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=SwathiSBhat/ThePulse" />
</a>
