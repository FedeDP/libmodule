.. |br| raw:: html

   <br />

Memory
======

Libmodule, even in its _easy (high level) APIs, is targeting a smart and hopefully large audience. |br|
Consequently, it is up to developers to understand and observe any pointers' scope. |br|
It means that any data passed to its API is trusted; it's up to implementer to actually be sure that eg: a registered topic pointer stays alive until it is needed. |br|
This is often accomplished by using global/static variables or heap-allocations. |br|

Libmodule chose to minimize internal heap allocations to avoid performance issues or enforcing a behaviour to developers, that in my opinion is much worse. |br|
Note that instead map and stack APIs are more flexible about it, offering to eg: duplicate hashmap keys. |br|
