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
mkdir -p inp
mkdir -p out

sudo mknod dev/kmsg c 1 11;
sudo mknod dev/console c 5 1;
sudo mknod dev/ram0 b 1 0;
sudo mknod -m 622 dev/console c 5 1
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/zero c 1 5
sudo mknod -m 666 dev/ptmx c 5 2
sudo mknod -m 666 dev/tty c 5 0
sudo mknod -m 444 dev/random c 1 8
sudo mknod -m 444 dev/urandom c 1 9
sudo mknod dev/tty0 c 4 0
sudo mknod dev/ttyS0 c 4 64
sudo mknod dev/ttyS1 c 4 65
sudo mknod dev/tty c 5 0
sudo mknod dev/console c 5 1
sudo mknod dev/ptmx c 5 2
sudo mknod dev/ttyprintk c 5 3
ln -s ../proc/self/fd fd
ln -s ../proc/self/fd/0 stdin # process i/o
ln -s ../proc/self/fd/1 stdout
ln -s ../proc/self/fd/2 stderr
ln -s ../proc/kcore     kcore
echo "hello fuzzing" > inp/01.txt
                             
find . -print0 | cpio --null --create --verbose --format=newc | gzip --best > ../fuzzcan-initramfs.cpio.gz
