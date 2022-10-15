#!/bin/sh
cd rootfs;
mkdir -p {dev, proc, sys, tmp, mnt, bin, usr, usr/lib, lib, etc, bin}
sudo mknod dev/console c 5 1;
sudo mknod dev/ram0 b 1 0;
find . -print0 | cpio --null --create --verbose --format=newc | gzip --best > ../fuzzcan-initramfs.cpio.gz
