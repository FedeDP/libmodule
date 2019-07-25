.. |br| raw:: html

   <br />

PubSub
======

Concepts
--------

Those unfamiliar with actor like messaging, may wonder what a pubsub messaging is. |br|
PubSub (Publisher-Subscriber) messaging is much like a producer/consumer architecture: an entity registers a "topic" on which it will send messages. |br|
Other entities can then subscribe to the topic to receive those messages. |br|

Implementation
--------------

Since libmodule 2.1, pubsub implementation is async and makes use of unix pipes. |br|
When sending a message to other modules, a pubsub message is allocated and its address is written in recipient module's writable end of pipe. |br|
The message will then get caught by modules_loop, the address read from readable end of pipe and callback called with the message. |br|

Since libmodule 4.0.0, module_tell() makes use of module references: it means recipient should be ref'd through module_ref(). |br|
Note that you cannot call any function on a module's reference as you cannot impersonate another module. |br|
Only module_is(), module_get_name/context() and module_poisonpill() functions can be called through a module's reference.

System messages
---------------

Beside USER messages (ps_msg_t.type), there are 6 system messages, respectively: LOOP_STARTED, LOOP_STOPPED, TOPIC_REGISTERED, TOPIC_DEREGISTERED, MODULE_STARTED, MODULE_STOPPED. |br|
These pubsub messages are automatically sent by libmodule when matching functions are called, eg: |br|
* LOOP_STARTED(STOPPED) is sent whenever a loop starts(stops) looping. It is useful to actually start(stop) your pubsub messagging (eg: one module waits on LOOP_STARTED to send a pubsub message to another module, and so on...). It won't have any valued fields, except for type. |br|
* TOPIC_(DE)REGISTERED message is sent when any module registers a topic; it is useful for other modules to (un)subscribe to it. |br|
It will have valued type and topic fields; moreover, sender field will be set to module that (de)registered the topic. |br|
* MODULE_STARTED(STOPPED) is sent whenever a module starts/resumes(stops/pauses). Again this is useful to inform other modules that a new module is available. |br|
It will have valued type and sender fields; sender will be set to started(stopped) module.

Finally, note that system messages with valued sender won't be sent to modules that actually generated the message.

Notes
-----

Note that a context must be looping to receive any pubsub message. |br|
Moreover, when a context stops looping, all pubsub messages will be flushed and thus delivered to each RUNNING module. |br|
Pubsub message sent while context is not looping or module is PAUSED are buffered until context starts looping/module gets resumed. For more information, see `pipe capacity <https://linux.die.net/man/7/pipe>`_. |br|
Finally, please be aware that data pointer sent through pubsub messaging is trusted, ie: you should pay attention to its scope.
