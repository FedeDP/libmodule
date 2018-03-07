all: lib example

debug: lib-debug example

lib-debug:
	cd Lib; $(CC) module.c -o libmodule.so -I. -shared -fpic  -Wl,--no-undefined -DDEBUG -Wshadow -Wtype-limits -Wstrict-overflow -fno-strict-aliasing -Wformat -Wformat-security -g -std=c99 -D_GNU_SOURCE -fvisibility=hidden

lib:
	cd Lib; $(CC) module.c -o libmodule.so -I. -shared -fpic -std=c99 -D_GNU_SOURCE -fvisibility=hidden

example:
	cd Sample; $(CC) *.c -L../Lib -lmodule -I../Lib -o sample.out -Wl,--no-undefined -g -std=c99 -D_GNU_SOURCE
