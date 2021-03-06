# Libmodule

[![builds.sr.ht status](https://builds.sr.ht/~fededp/libmodule.svg)](https://builds.sr.ht/~fededp/libmodule?)
[![Documentation Status](https://readthedocs.org/projects/libmodule/badge/?version=latest)](http://libmodule.readthedocs.io/en/latest/?badge=latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## What is this?

Libmodule offers a small and simple C implementation of an actor library that aims to let developers easily create modular C projects in a way which is both simple and elegant.  
Indeed, libmodule was heavily inspired by my own actor library experience with [akka](https://akka.io/) for its API.  

## What is a module, anyway?

Unsurprisingly, module is the core concept of libmodule architecture.  
A module is an Actor that can listen on socket events too.  
Frankly speaking, it is denoted by a MODULE() macro plus a bunch of mandatory callbacks, eg:
```C
#include <module/module_easy.h>
#include <module/modules_easy.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

MODULE("Pippo");

static void init(void) {
    /* Register STDIN fd, without autoclosing it at the end */
    m_register_fd(STDIN_FILENO, false, NULL);
}

static bool check(void) {
    /* Should module be registered? */
    return true;
}

static bool evaluate(void) {
    /* Should module be started? */
    return true;
}

static void destroy(void) {
    
}

static void receive(const msg_t *msg, const void *userdata) {
    if (!msg->is_pubsub) {
        char c;
        read(msg->fd_msg->fd, &c, sizeof(char));
        switch (tolower(c)) {
            case 'q':
                m_log("Leaving...\n");
                m_tell_str(self(), "ByeBye");
                break;
            default:
                if (c != ' ' && c != '\n') {
                    m_log("Pressed %c\n", c);
                }
                break;
        }
    } else if (msg->pubsub_msg->type == USER && 
        !strcmp((char *)msg->pubsub_msg->message, "ByeBye")) {
            
        modules_quit(0);
    }
}
```
In this example, a "Pippo" module is created and will read chars from stdin until 'q' is pressed.  
Note that it does not even need a main function, as libmodule already provides a default one as a [weak, thus overridable, symbol](https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Function-Attributes.html).  

## Is it portable?

Yes, it is. Non-portable code is actually [compile-time-plugins](https://github.com/FedeDP/libmodule/tree/master/Lib/poll_plugins) based.  
On linux, libmodule's internal loop will use epoll, while on BSD and MacOS it will use kqueue.  
On other OS, cmake will fallback at looking for [libkqueue](https://github.com/mheily/libkqueue), a drop-in replacement for kqueue.  
Unfortunately, I am not able to test builds on other OS: I could only check that libmodule can be built on Linux through libkqueue.  

Finally, it heavily relies upon gcc attributes that may or may not be available for your compiler.

## Is there any documentation?

Yes, it is availabe at [readthedocs](http://libmodule.readthedocs.io/en/latest/).  
You have some simple examples too, check [Samples](https://github.com/FedeDP/libmodule/tree/master/Samples) folder.  
To see a real project using libmodule, check [Clightd](https://github.com/FedeDP/Clightd).

## CI

Libmodule, samples and tests builds are tested on [builds.sr.ht](https://builds.sr.ht/~fededp/libmodule) on archlinux, ubuntu, fedora and freebsd.  
Moreover, tests are executed and valgrind checked too.  

## But...why?

We all know OOP is not a solution to every problem and C is still a beautiful and much used language.  
Still, I admit to love code modularity that OOP enforces; moreover, I realized that I was using same code abstractions over and over in my C projects (both side projects and at my job).  
So I thought that writing a library to achieve those same abstractions in a cleaner and simpler way was the right thing to do and could help some other dev. 

## Build dep and how to build

You only need cmake to build libmodule on Linux and BSD/osx; it does not depend upon external software.  
On other platforms, you will need [libkqueue](https://github.com/mheily/libkqueue) too.  
To build, you only need to issue:

    $ mkdir build
    $ cd build
    $ cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=lib ../
    $ make

*Note that libmodule can also be built as static library, by passing -DSTATIC_MODULE=true parameter to cmake.*

Installation - Generic OS
-------------------------

    # make install

Installation - Red Hat
----------------------

    $ cpack -G RPM

And finally install generated RPM package.

Installation - Debian
---------------------

    $ cpack -G DEB

And finally install generated DEB package.

Libmodule will install a pkg-config script too: use it to link libmodule in your projects, or use "-lmodule" linker flag.  
Please note that in order to test examples, there is no need to install the library.

For Archlinux users, Libmodule is available on [AUR](https://aur.archlinux.org/packages/libmodule/).

## License
Libmodule is made available with a MIT license to encourage people to actually try it out and maybe use it inside their project, no matter if open or closed source.  

*Are you using libmodule in your project? Please let me know and I will gladly list it here.*
