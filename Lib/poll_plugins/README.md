## Poll plugins

In this folder you can find poll plugins to support various OS.  
As of today, an epoll plugin (for Linux) and a kqueue plugin (for BSD and MacOS) are provided.  
Each plugin must implement and provide [poll_priv](https://github.com/FedeDP/libmodule/blob/kqueue_support/Lib/poll_priv.h) interface.  
As you can see, provided plugins have to offer an epoll-like API.  
In [CMakeLists.txt](https://github.com/FedeDP/libmodule/blob/kqueue_support/CMakeLists.txt#L12) correct plugin for target OS is built.  

*Any pull request to expand libmodule's availability is warmly welcomed.*
