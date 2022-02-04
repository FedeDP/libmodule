# Module

A module is the core entity of libmodule: it is a single and indipendent logical unit that reacts to events.  
It can be seen as an Actor with the ability of managing non-PubSub events.  
It requires some callbacks that are used by libmodule to manage its life.  

## Source

Each module's listens to events by registering event sources.  
A source can be a timer, a topic (for PubSub communications between modules), a signal, a socket, etc etc.  
You can find the list of supported sources in `<module/mod.h>`.   

### Source Priorities

TODO

## Lifecycle

### Callbacks

First of all, module lifecycle is automatically managed by libmodule; moreover, when using `<module/mod_easy>` API,
module registration/deregistration is completely automated and transparent to developer.  
This means that when using it, you will only have to declare a source file as a module and define needed functions.  

First function to be called is `m_pre_start()`; it is called right after libmodule gets linked.  
This function is useful to set a custom memhook for libmodule, through `m_set_memhook()` API.  

> **_EASY API_**: each module's `m_mod_pre_start()` function is called. At this stage, no module is registered yet.  

Finally, libmodule will register every module.  
Once a module is registered, it will be initially set to an M_MOD_IDLE state. Idle means that the module is not started yet, thus it cannot receive any event.  

As soon as its context starts looping, `m_mod_on_eval()` function will be called on every M_MOD_IDLE module, trying to start it right away.  
That function will be called at each state machine change, for each idle module, until it returns true.  

As soon as `m_mod_on_eval()` returns true, a module is started. It means its `m_mod_on_start()` function is finally called and its state is set to M_MOD_RUNNING.  
When a module reaches RUNNING state, its context loop will finally receive events for its registered sources.  

Whenever an event triggers a module's wakeup, its `m_mod_on_evt()` callback can be called (depending on the event source priority) with a `m_queue_t<m_evt_t *>` argument.  

Finally, when stopping a module, its `m_mod_on_stop()` function is called.  
Note that a module is automatically stopped during the process of module's deregistration.  

Thus, for Easy API, you should only implement m_mod_on_eval() to return true when it is ready to be started,  
then eventually register event sources in m_mod_on_start(), and manage events in m_mod_on_evt().  
If you need to cleanup any data, manage it in m_mod_on_stop().  

### States

As previously mentioned, a registered module, before being started, is in M_MOD_IDLE state.  
M_MOD_IDLE state means that it has never been started before; it won't receive any event thus its m_evt_cb will never be called.  
When module is started, thus reaching M_MOD_RUNNING state, all its registered sources will finally start to receive events. Sources registered while in M_MOD_RUNNING state are automatically polled.  
If a module is paused, reaching M_MOD_PAUSED state, it will stop polling on its event sources, but event sources will still be alive, just not polled. Thus, as soon as module is resumed, all events received during PAUSED state will trigger m_evt_cb.  
If a module gets stopped, reaching M_MOD_STOPPED state, it will destroy any registered source and it will be resetted to its initial state.  
If you instead wish to stop a module letting it manage any already-enqueued event, you need to send a POISONPILL message to it, through `m_mod_poisonpill()` API.  
The difference between M_MOD_IDLE and M_MOD_STOPPED states is that M_MOD_IDLE modules will be evaluated to be started during context loop, while M_MOD_STOPPED modules won't.  
When a module is deregistered, it will reach a final M_MOD_ZOMBIE state. It means that the module is no more reachable neither usable, but it can still be referenced by any previously sent message (or any `m_mod_ref()`).  
After all module's ref count drops to 0 (ie: all sent messages are received by respective recipients and there are no pending `m_mod_unref()`) module will finally get destroyed and its memory freed.  
You can only call `m_mod_is()`, `m_mod_name()` and `m_mod_ctx()` on a zombie module.  

To summarize:  
* `m_mod_start()` needs to be called on an M_MOD_IDLE or M_MOD_STOPPED module.  
* `m_mod_pause()` needs to be called on a M_MOD_RUNNING module.  
* `m_mod_resume()` needs to  be called on a M_MOD_PAUSED module.  
* `m_mod_stop()` needs to be called on a M_MOD_RUNNING or M_MOD_PAUSED module.  