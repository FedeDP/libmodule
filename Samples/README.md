# Libmodule Samples

This folder contains some libmodule's examples.  
For now, they are the only API documentation available too.  
As you can see, API is quite straightforward though.

To build these samples, use 

    $ make $target
    
where $target matches the name of the subfolder for your desired example.

## Easy example

[Easy](https://github.com/FedeDP/libmodule/tree/master/Sample/Easy) example shows how to use simple, single-context libmodule API.  
This is the simplest libmodule usage example, and it is self-explanatory.

## Shared source example

[SharedSrc](https://github.com/FedeDP/libmodule/tree/master/Sample/SharedSrc) example shows how to use "more complicated" libmodule API to  
create 2 modules inside same source file, that can shared some callbacks too.  
This is highly discouraged though, as main libmodule aim is to create simple and modular C projects.  
There can be some cases, though, were eg: 2 modules share a huge callback that you may not desire to be copied in 2 different files.

## Multi context example

[MultiCtx](https://github.com/FedeDP/libmodule/tree/master/Sample/MultiCtx) example shows how to use "most complicated" libmodule API.  
This example fully introduces context's concept, already seen in previous example.  
A context is a way to create subnets of modules. You can call modules_loop on each context, and each context behaves independently from others.  
This can be particularly useful when dealing with 2+ threads; ideally, each thread has its own module's context.
