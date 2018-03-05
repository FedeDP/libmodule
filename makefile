CC=gcc

all: lib example

lib:
	$(CC) modules.c -o libmodules.so -shared -fpic  -Wl,--no-undefined -g

example:
	cd Sample; $(CC) *.c -L../ -lmodules -I../ -o sample.out -Wl,--no-undefined -g
