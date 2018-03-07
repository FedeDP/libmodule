# Libmodule

[![Build Status](https://travis-ci.org/FedeDP/libmodule.svg?branch=master)](https://travis-ci.org/FedeDP/libmodule)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3526dd92b6d84370b072bfadfc7da632)](https://www.codacy.com/app/FedeDP/libmodule?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=FedeDP/libmodule&amp;utm_campaign=Badge_Grade)

Libmodule aims at let developers easily create modular C projects in a way that is both simple and elegant.  
You will write less code, focusing on what you really need.  

## Is this an event loop or an actor lib?

Well, it is both, someway.  
It stands somewhere in the middle: it does not provide any faciliting to build an event loop; it does provide its own event loop though.  
You may find some/lots of similarities between a libmodule MODULE and an Actor.  
Indeed, libmodule was heavily inspired by my own actor library experience with [akka](https://akka.io/).  
It can probably be expanded to an actor library with some work. I may even work on that at a certain point.

## Is it portable?

No, it is not.  
It uses epoll, that is linux specific.  
Moreover it heavily relies upon gcc attributes that may or may not be available for your compiler.  
It is tested with both gcc and clang with [travis](https://travis-ci.org/FedeDP/libmodule).

## Is there any documentation?

Right now no. I'm sorry.  
I'm focusing about providing as much features as possible, and the project is under heavy development.  
I promise that as soon as it is stable enough, documentation will be top priority (and provided asap).  
For now you have to rely on my [sample](https://github.com/FedeDP/libmodule/tree/master/Sample).
