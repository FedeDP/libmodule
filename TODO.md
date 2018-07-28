# TODO

## 2.1

### Async nessage handling

- [x] all PubSub functions should malloc the pubsubmsg and writes its address in a pipe.
- [x] Each module has a pipe associated
- [x] The module gets awaken by the pipe and its recv() call is called
- [x] fix possible memleaks when leaving if some pubsub msg is still queued (flush all queued messages)
- [x] Fix easy sample when leaving (doggo does not recveive last m_publish())
- [x] use pipe2 on linux, and pipe + fnctl on osx/bsd (set fds to NON_BLOCKING)
- [x] Add a pubsub msg sent to all modules in a ctx when the ctx starts/stops looping
- [x] PubSub system only works on looping context
- [x] Publishing message only works on existent topic
- [x] Only module which created a topic can publish on that topic
- [x] Add easy api defines
- [x] Do not require a looping context to send pubsub message. Messages will be delivered as soon as context starts looping.
- [x] When stopping a module, close its write side of pipe to avoid memleak if it gets restarted.

### API improvements

- [x] add module_unsubscribe
- [x] add module_register_topic 
- [x] add module_deregister_topic

- [x] Add special system-pubsub messages when: loop is started, loop is stopped, topic is registered, topic is deregistered

- [x] Add module_unsubscribe/module_(de)register_topic tests
- [x] fix tests
- [x] fix memleaks

- [x] Add/rm fd for stopped modules too.

- [x] Fixed issues with module_stop that did not cleanly wiped module->fds, preventing a module_start to be successful

- [x] Forcefully rm all fds when de-registering a module: if one stops a module, then adds some fds without starting it, then de-registers it, we would have memleaks

- [ ] Update examples
- [ ] Update doc: now modules_loop is always needed, even in case on pubsub only messaging

- [ ] Release 2.1

## 2.2/3.0

### Remote modules concept

- [ ] Add "type" enum in self_t -> LOCAL, REMOTE
- [ ] Add a MODULE_REMOTE() macro
- [ ] Remote modules work as bus between 2+ remote libmodule instances. When you send a message to a remote module, it will be forwarded to real remote module
- [ ] Study feasibility...
- [ ] REST like api? Wrap remote messages in json through jansson? (https://babelouest.github.io/ulfius/ ?)

- [ ] UPDATE DOC + release

## Later/Ideas

### Dep system (??)

- [ ] REQUIRE and AFTER macros?

- [ ] Release

### Submodules (??)

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

- [ ] Release
