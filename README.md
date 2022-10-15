Totally Stolen from: A gentle introduction to Linux Kernel fuzzing
=============================================

Requirements
-------------
qemu-kvm, kernel development tools, static busybox.

Compiling
--------------
First compile a linux-5.19.3 kernel using the 
provided config file (place it in the linux kernel
source dir as .config).
To create the initird
	make rootfs
should build fuzzcan, place it in the initramfs and 
then create a new initramfs at the current directory.

Running
---------------

qemu-system-x86_64 -kernel /path/to/bzImage -initrd /path/tofuzzcan-initramfs.cpio.gz \
 -append "root=/dev/ram0 rootfstype=ramfs init=/init console=ttyS0" -net nic,model=rtl8139 \
 -net user -m 2048M --nographic

