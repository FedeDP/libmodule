# TODO

## 2.0

### Small API improvements

- [x] Pass a self_t instead of const char *name/context in various exposed function
- [x] Add a module_get_name/context(self_t *self) that returns a strdupped string

### Fixes

- [x] fix library -> module_cmn.h should be a public header
- [x] fix MODULE_VERSION_MAJ, MODULE_VERSION_MIN, MODULE_VERSION_PAT export
- [x] BUILD_TESTS instead of checking for CMAKE_BUILD_TYPE
- [x] define MODULE_MAX_EVENTS in module_cmn.h interface 
- [x] offer a new modules_loop that takes a max_events parameter.
- [x] move public interface to public/module/, this way i can use normal path includes while building tests and samples
- [x] Add a readme.md in public/module.
- [x] Add a cpp example
- [x] Drop unused code

- [ ] Switch to uint8 etc etc where needed (eg: c->quit, msg->is_pubsub etc etc)
- [ ] Change license to Apache 2.0

### Mem Alloc functions customizable

- [x] add a modules_set_memhook function to let user define memory allocator/deallocator functions. Defaults to malloc/realloc/free

### Async nessage handling

- [ ] all PubSub functions should start a new thread that mallocs the pubsubmsg and writes its address in a pipe. (is the thread really needed?)
- [ ] Each module has a pipe 
- [ ] The module gets awaken by the pipe and its recv() call is called
- [ ] This way PubSub messaging is not blocking (ie module A tells module B something, and module B, in his recv, starts waiting on something else. A is locked until B recv() function exits)

- [ ] UPDATE DOC + document MODULE_VERSION_MAJ/MIN/PATCH + release 2.0

## 2.1/3.0

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
