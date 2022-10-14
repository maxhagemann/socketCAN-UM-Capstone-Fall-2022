#!/bin/sh
cd rootfs;
find . -print0 | cpio --null --create --verbose --format=newc | gzip --best > ../fuzzcan-initramfs.cpio.gz
