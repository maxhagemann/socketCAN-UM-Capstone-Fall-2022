fuzzcan: src/*h src/*c Makefile
	gcc -g -O2 -Wall -Wextra \
		src/utils.c \
		src/kcov.c \
		src/forksrv.c \
		src/siphash.c \
		src/namespace.c \
		src/fuzzcan.c \
		-o $@

.PHONY: format
format:
	clang-format -i src/*.c src/*.h
