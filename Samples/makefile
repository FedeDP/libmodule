.PHONY: Easy SharedSrc MultiCtx

all: Easy SharedSrc MultiCtx

Easy:
	cd Easy; $(CC) *.c -L../../build -lmodule -I../../Lib -o sample.out -Wl,-rpath=../../build

SharedSrc:
	cd SharedSrc; $(CC) *.c -L../../build -lmodule -I../../Lib -o sample.out -Wl,-rpath=../../build

MultiCtx:
	cd MultiCtx; $(CC) *.c -L../../build -lmodule -lpthread -I../../Lib -o sample.out -Wl,-rpath=../../build
