# TODO

## 2.0

- [x] Release 2.0

## 2.1

### Async nessage handling

- [ ] all PubSub functions should that malloc the pubsubmsg and writes its address in a pipe.
- [ ] Each module has a pipe 
- [ ] The module gets awaken by the pipe and its recv() call is called
- [ ] This way PubSub messaging is not blocking (ie module A tells module B something, and module B, in his recv, starts waiting on something else. A is locked until B recv() function exits)

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
