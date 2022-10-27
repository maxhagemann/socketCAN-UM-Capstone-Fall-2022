# will compile a fuzzcan shim and place it
# in the rootfs dir.

fuzzcan: src/*h src/*c Makefile
	gcc -static -g -O2 -Wall -Wextra \
		src/utils.c \
		src/kcov.c \
		src/forksrv.c \
		src/siphash.c \
		src/fuzzcan.c \
		-o $@

rootfs: fuzzcan
	cp fuzzcan rootfs/bin/
	./mkrootfs.sh

.PHONY: format
format:
	clang-format -i src/*.c src/*.h

clean:
	rm fuzzcan;
	rm -rf rootfs/bin/fuzzcan
