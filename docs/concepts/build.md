# Build

Libmodule supports multiple OSes:
* Linux
* freebsd
* osx

Non-portable code is compile-time-plugins based.  
On linux, libmodule's internal loop will use `epoll`, while on BSD and MacOS it will use `kqueue`.  

on linux, one can also enforce the usage of [`libkqueue`](https://github.com/mheily/libkqueue), a drop-in userspace replacement for kqueue;  
moreover, an experimental `liburing` backend is also available.  

Libmodule uses [`CMake`](https://cmake.org/) as a build tool.  
To build it, you just need to issue:
```shell
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Generic Options

Libmodule supports the following build switches:
* `BUILD_TESTS` -> whether to build tests. Requires `cmocka` library.
* `BUILD_SAMPLES` -> whether to build examples.
* `BUILD_OOT_TEST` -> whether to build a small out of tree test, to check if libmodule can successfully get linked. The test is done during the `make install` target.  

## Core library Options

Libmodule core library supports the following build switches:
* `WITH_FS` -> whether to enable a fuse FS layer. Requires `fuse3` library.
* `WITH_LIBURING` (linux-only) -> whether to enable and use liburing poll plugin.
* `WITH_LIBKQUEUE` (linux-only) -> whether to enable and use kqueue poll plugin using libkqueue library.
