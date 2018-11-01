.. |br| raw:: html

   <br />

Libmodule PubSub interface
==========================

PubSub concept
--------------

Those unfamiliar with actor like messaging, may wonder what a pubsub messaging is. |br|
PubSub (Publisher-Subscriber) messaging is much like a producer/consumer architecture: an entity registers a "topic" on which it will send messages. |br|
Other entities can then subscribe to the topic to receive those messages. |br|

PubSub implementation
---------------------

Since libmodule 2.1, pubsub implementation is async and makes use of unix pipes. |br|
When sending a message to other modules, a pubsub message is allocated and its address is written in recipient module's writable end of pipe. |br|
The message will then get caught by modules_loop, the address read from readable end of pipe and callback called with the message.

PubSub system messages
----------------------

Beside USER messages (pubsub_msg_t.type), there are 4 system messages, with type respectively: LOOP_STARTED, LOOP_STOPPED, TOPIC_REGISTERED, TOPIC_DEREGISTERED. |br|
These pubsub messages are automatically sent by libmodule (note that sender will be NULL) when matching functions are called. |br|
For example, you can use TOPIC_REGISTERED message (note that pubsub_msg_t.topic will be valued matching newly created topic) to subscribe to a topic as soon as it appears in current context.

PubSub notes
------------

Note that a context must be looping to receive any pubsub message. |br|
Moreover, when a context stops looping, all pubsub messages will be flushed and thus delivered to each RUNNING module. |br|
Pubsub message sent while context is not looping are buffered until context starts looping. For more information, see `pipe capacity <https://linux.die.net/man/7/pipe>`_. |br|
Finally, please be aware that data pointer sent through pubsub messaging is trusted, ie: you should pay attention to its scope.
