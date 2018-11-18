# Libmodule

[![builds.sr.ht status](https://builds.sr.ht/~fededp/libmodule.svg)](https://builds.sr.ht/~fededp/libmodule?)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3526dd92b6d84370b072bfadfc7da632)](https://www.codacy.com/app/FedeDP/libmodule?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=FedeDP/libmodule&amp;utm_campaign=Badge_Grade)
[![Documentation Status](https://readthedocs.org/projects/libmodule/badge/?version=latest)](http://libmodule.readthedocs.io/en/latest/?badge=latest)
[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

Libmodule is a C library targeting linux aiming to let developers easily create modular C projects in a way which is both simple and elegant.  
You will write less code, focusing on what really matters.  

**Please note that libmodule actually builds and works on macOS and BSD too, and probably other.** See [Portability](https://github.com/FedeDP/libmodule#is-it-portable).

## Is this an event loop or an actor lib?

It stands somewhere in the middle, trying to mix the 2 concepts.  
It does not provide any faciliting to build an event loop; it does provide its own event loop though.  
You may find some/lots of similarities between a libmodule's *Module* and an Actor.  
Indeed, libmodule was heavily inspired by my own actor library experience with [akka](https://akka.io/) for its API.  

## Is it portable?

Yes, it is. Non-portable code is actually [compile-time-plugins](https://github.com/FedeDP/libmodule/tree/master/Lib/poll_plugins) based.  
On linux, libmodule's internal loop will use epoll, while on BSD and MacOS it will use kqueue.  
On other OS, cmake will fallback at looking for [libkqueue](https://github.com/mheily/libkqueue), a drop-in replacement for kqueue.  
Unfortunately, I am not able to test builds on other OS: I could only check that libmodule can be built on Linux through libkqueue.  
*If anyone is interested in step up and test/maintain libmodule on non-linux platforms, I'd be very thankful*.  

Finally, it heavily relies upon gcc attributes that may or may not be available for your compiler.  

## Is there any documentation?

Yes, it is availabe at [readthedocs](http://libmodule.readthedocs.io/en/latest/).  
You have some nice examples too, check [Samples](https://github.com/FedeDP/libmodule/tree/master/Samples) folder.  
To see a real project using libmodule, check [Clightd](https://github.com/FedeDP/Clightd).

## CI

Libmodule, samples and tests builds are tested on [builds.sr.ht](https://builds.sr.ht/~fededp/libmodule) on linux and freebsd.  
Moreover, tests are executed and valgrind checked too.  

## What is a module, anyway?

Unsurprisingly, module is the core concept of libmodule architecture.  
It can be somewhat seen as a class, and shares lots of concepts with an Actor.  
It helps you to write standard and clean projects with small units, so called modules, whose job should be self-contained.  

## But...why?

We all know OOP is not a solution to every problem and C is still a beautiful and much used language.  
Still, I admit to love code modularity that OOP enforces; moreover, I realized that I was using same code abstractions over and over in my C projects (both side projects and at my job).  
So I thought that writing a library to achieve those same abstractions in a cleaner and simpler way was the right thing to do.

## Build dep and how to build

You only need cmake to build libmodule; it does not depend upon external software.  
To build, you only need to issue:

    $ mkdir build
    $ cd build
    $ cmake ../
    $ make

If you wish to install, then you only need:

    # make install

Libmodule will install a pkg-config script too: use it to link libmodule in your projects, or use "-lmodule" linker flag.  
Please note that in order to test examples, there is no need to install the library.

For Archlinux users, Libmodule is available on [AUR](https://aur.archlinux.org/packages/libmodule/).

