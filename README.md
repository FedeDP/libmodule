# Libmodule

[![Build Status](https://travis-ci.org/FedeDP/libmodule.svg?branch=master)](https://travis-ci.org/FedeDP/libmodule)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3526dd92b6d84370b072bfadfc7da632)](https://www.codacy.com/app/FedeDP/libmodule?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=FedeDP/libmodule&amp;utm_campaign=Badge_Grade)
[![Documentation Status](https://readthedocs.org/projects/libmodule/badge/?version=latest)](http://libmodule.readthedocs.io/en/latest/?badge=latest)

Libmodule aims to let developers easily create modular C projects in a way that is both simple and elegant.  
You will write less code, focusing on what you really need.  

## Is this an event loop or an actor lib?

It stands somewhere in the middle, trying to mix the 2 concepts.  
It does not provide any faciliting to build an event loop; it does provide its own event loop though.  
You may find some/lots of similarities between a libmodule's MODULE and an Actor.  
Indeed, libmodule was heavily inspired by my own actor library experience with [akka](https://akka.io/) for its API.  

## Is it portable?

No, it is not.  
It uses epoll, that is linux specific.  
Moreover it heavily relies upon gcc attributes that may or may not be available for your compiler.  
It is tested with both gcc and clang through [travis](https://travis-ci.org/FedeDP/libmodule).  
I may consider and welcome any patch to support other platforms though.  

## Is there any documentation?

Yes, it is availabe at http://libmodule.readthedocs.io/en/latest/.  
You have some nice examples too, check [Samples](https://github.com/FedeDP/libmodule/tree/master/Samples) folder.

## What is a module, anyway?

Unsurprisingly, module is the core concept of libmodule architecture.  
It can be somewhat seen as a class, and shares lots of concepts with an Actor.  
It helps you to write standard and clean projects with small units, so called modules, whose job should be self-contained.  

## But...why?

We all know OOP is not a solution to everything and C is still a beautiful and much used language.  
Still, I admit to love code modularity that OOP enforces; moreover, I realized that I was using same code and abstractions over and over in my C projects (both side projects and at my job).  
So I thought that writing a library to achieve those same abstractions in a cleaner and simpler way was the right thing to do.

## Build dep and how to build

You only need cmake to build libmodule.  
It does not depend upon external software.  
Libmodule includes an [hashmap](https://github.com/petewarden/c_hashmap) implementation provided by Pete Warden (thank you!).  
To build, you only need to issue:

    $ mkdir build
    $ cd build
    $ cmake ../
    $ make

If you wish to install, then you only need:

    # make install

Libmodule will install a pkg-config file too. Use this to link libmodule in your projects, or use "-lmodule" flag.  
Please note that in order to test examples, there is no need to install the library.

For Archlinux users, a PKGBUILD can be found in [Extra/Arch](https://github.com/FedeDP/libmodule/tree/master/Extra/Arch) folder.

