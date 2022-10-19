#!/bin/sh
cd rootfs;
mkdir -p dev
mkdir -p proc
mkdir -p sys
mkdir -p tmp 
mkdir -p mnt
mkdir -p usr
mkdir -p usr/lib
mkdir -p lib
mkdir -p etc
mkdir -p bin

sudo mknod dev/kmsg c 1 11;
sudo mknod dev/console c 5 1;
sudo mknod dev/ram0 b 1 0;
find . -print0 | cpio --null --create --verbose --format=newc | gzip --best > ../fuzzcan-initramfs.cpio.gz
