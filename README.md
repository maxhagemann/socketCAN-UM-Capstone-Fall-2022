No Longer Totally Stolen from: A gentle introduction to Linux Kernel fuzzing
=============================================
Some useful resource material for this project 
------------
Project we started working from - https://blog.cloudflare.com/a-gentle-introduction-to-linux-kernel-fuzzing/



General linux kernel intro - https://linuxconfig.org/in-depth-howto-on-linux-kernel-configuration



CAN tutorial - https://sgframework.readthedocs.io/en/latest/cantutorial.html



CAN info - https://www.kernel.org/doc/Documentation/networking/can.txt



AFL++ - https://github.com/AFLplusplus/AFLplusplus

Requirements
-------------
### Before anything, its always a good idea to check if you have the latest repository first! :)

	sudo apt update && upgrade

### I. Install QEMU-KVM:

	sudo apt install qemu-kvm
	
### II. Install some kernel development tools:

	sudo apt install libncurses-dev flex bison openssl libssl-dev dkms libelf-dev libudev-dev libpci-dev libiberty-dev autoconf
	
### III. Install Busybox STATICALLY:

	sudo apt install busybox -y
	
### IV. Install our Linux Kernel Tar (This project uses 5.19.7)

	https://www.kernel.org/pub/linux/kernel/v5.x/linux-5.19.7.tar.xz
	
### V. Install AFL++ and **COMPILE IT STATICALLY**, Instructions can be found here https://github.com/AFLplusplus/AFLplusplus/blob/stable/docs/INSTALL.md

	sudo apt-get install -y build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev
	# try to install llvm 12 and install the distro default if that fails
	sudo apt-get install -y lld-12 llvm-12 llvm-12-dev clang-12 || sudo apt-get install -y lld llvm llvm-dev clang
	sudo apt-get install -y gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
	sudo apt-get install -y ninja-build # for QEMU mode
	git clone https://github.com/AFLplusplus/AFLplusplus
	cd AFLplusplus
	make distrib
	sudo make install

Setup
--------------
	
### KERNEL BUILD: 

Untar the downloaded kernel file from step IV. and run this script to add KCOV instrument for code coverage before building new kernel:

	find net -name Makefile | xargs -L1 -I {} bash -c 'echo "KCOV_INSTRUMENT := y" >> {}'

Before compiling, replace the `.config` file in the Linux source dir with the included config file from the `confs` directory, 
be sure it is renamed correctly to `.config`


You're most likely not going to be able to see the `.config` file by default, so you have to resort to using the terminal and deleting it
(just use `ls -a` and delete `.config` , then replace it with given one)
 
Compile the new Linux kernel with this config file. From inside the linux untarred linux-5.19.7 folder:

	make


### SETUP FUZZCAN, ROOTFS, ETC.:

At this point, you should have the afl-fuzz binary ready (from building AFL++ statically as described in step V.) Copy that binary into `rootfs`


**and then run `make rootfs` and `./mkrootfs.sh`** **in this specific order**



After having the fuzzcan-initramfs.cpio.gz file, the `/bin/` directory should have 3 things being `afl-fuzz`, `fuzzcan`, and `busybox`.



![Screenshot_3](https://user-images.githubusercontent.com/22306262/205425896-6a549edd-98b8-4ad0-b047-e6ae486eac40.jpg)



take the .gz file and put it into the kernel directory.


![Screenshot_4](https://user-images.githubusercontent.com/22306262/205426106-509018d4-b3f2-41be-b954-102bcce7b1db.jpg)


Now, inside your kernel directory, make a 1G qemu raw img to save the fuzzing state

	dd if=/dev/zero of=/path/to/vmimg.raw bs=1M count=1000 

then, format it as ext3

	mkfs.ext3 /path/to/vmimg.raw
	
Now mount it! Note that you will first need to make a directory as a mount point. Best do it in your working folder with
	
	mkdir <name of mount dir>

And mount:
	
	sudo mount vmimg.raw /path/to/mount/dir


Running
---------------

Launch qemu:

	qemu-system-x86_64 -kernel /path/to/bzImage -initrd /path/tofuzzcan-initramfs.cpio.gz \ 
	-device virtio-scsi-pci,id=scsi -device scsi-hd,drive=hd -drive file=/path/to/vmimg.raw,if=none,format=raw,id=hd
	-append "root=/dev/ram0 rootfstype=ramfs init=/init console=ttyS0" -net nic,model=rtl8139 \
 	-net user -m 2048M --nographic
(^^^ careful! .not the bzImage that is in kernelsrc/arch/x86_64/boot. ^^^
The vmlinux (the uncompressed pure kernel binary. is just under kernelsrc/)


Running AFL++:

to run afl in the vm run the following command:

	./afl-fuzz -i inp -o out -m 1024 -t 4000 -- ./bin/fuzzcan 


if that doesn't seem to work for some reason, try this:

	afl-fuzz -i inp/ -o out/ -- ./bin/fuzzcan


To 'properly' quit qemu as the typical ctrl+c doesn't work here for some reason:

	pkill -9 qemu



	

