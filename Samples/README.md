# Libmodule Samples

This folder contains some libmodule's examples.  
As you can see, API is quite straightforward as being clean and simple has been one of the library's focus.

To build these samples, use 

    $ make $target
    
where $target matches the name of the subfolder for your desired example.

## Easy example

[Easy](https://github.com/FedeDP/libmodule/tree/master/Samples/Easy) example shows how to use simple, single-context libmodule API.  
This is the simplest libmodule usage example, and it is self-explanatory.

## Shared source example

[SharedSrc](https://github.com/FedeDP/libmodule/tree/master/Samples/SharedSrc) example shows how to use "more complicated" libmodule API to create 2 modules inside same source file that share some callbacks.  
This is highly discouraged though, as main libmodule aim is to create simple and modular C projects.  
There can be some cases, though, were 2 modules share a huge callback that you may not desire to have copied in 2 different source files.

## Multi context example

[MultiCtx](https://github.com/FedeDP/libmodule/tree/master/Samples/MultiCtx) example shows how to use "most complicated" libmodule API.  
This example fully introduces context's concept, already seen in previous example.  
