No Longer Totally Stolen from: A gentle introduction to Linux Kernel fuzzing
=============================================

Requirements
-------------
qemu-kvm, kernel development tools, static busybox, Linux kernel tar - this project uses 5.19.7.


Compiling
--------------

Download kernel tar from: https://www.kernel.org/pub/linux/kernel/v5.x/linux-5.19.7.tar.xz

Untar, run this script to add KCOV instrument for code coverage before building new kernel:

	find net -name Makefile | xargs -L1 -I {} bash -c 'echo "KCOV_INSTRUMENT := y" >> {}'

Before compiling, replace the .config file in the Linux source dir with the included config
file from the confs directory, be sure it is renamed correctly to ".config".
 
Compile the new Linux kernel with this config file.

To compile AFL++ statically, we can do it by using the following command
	
	make STATIC=1 all

This should have the afl-fuzz binary ready. Copy that binary into rootfs **and then run `make rootfs` and `./mkrootfs.sh`**
After having the fuzzcan-initramfs.cpio.gz file, the `/bin/` directory should have 3 things being afl-fuzz, fuzzcan, and busybox.

take the .gz file and put it into the kernel directory.

To create the initird

	make rootfs

should build fuzzcan, place it in the initramfs and 
then create a new initramfs at the current directory.




Running
---------------

Make a 1G qemu raw img to save the fuzzing state

	dd if=/dev/zero of=/path/to/vmimg.raw bs=1M count=1000 

format it as ext3

	mkfs.ext3 /path/to/vmimg.raw

Launch qemu:

	qemu-system-x86_64 -kernel /path/to/bzImage -initrd /path/tofuzzcan-initramfs.cpio.gz \ 
	-device virtio-scsi-pci,id=scsi -device scsi-hd,drive=hd -drive file=/path/to/vmimg.raw,if=none,format=raw,id=hd
	-append "root=/dev/ram0 rootfstype=ramfs init=/init console=ttyS0" -net nic,model=rtl8139 \
 	-net user -m 2048M --nographic
(^^^ careful! .not the bzImage that is in kernelsrc/arch/x86_64/boot. ^^^
The vmlinux (the uncompressed pure kernel binary. is just under kernelsrc/)


To launch the fuzzer run

	./afl-fuzz -i inp/ -o /mnt/out -- ./bin/fuzzcan

/* You may have to press "enter" again after executing. This should yield a stream of hex pointers. 
To view these in user format: create a text file, copy the data stream and paste into this file.
Save it and run this file through addr2line:

	cat output_file | addr2line -e /path/to/vmlinux  
*/

Running AFL++:

to run afl in the vm run the following command:

`./afl-fuzz -i inp -o out -m 1024 -t 4000 -- ./bin/fuzzcan` 


if that doesn't seem to work for some reason, try this:

`afl-fuzz -i inp/ -o out/ -- ./bin/fuzzcan`


To propely quit qemu:

	pkill -9 qemu
	

