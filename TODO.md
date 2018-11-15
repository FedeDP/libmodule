## 3.1.0

### Generic
- [x] Add readme mention of Clightd, project that is using libmodule
- [x] Expose module_map_{create/put/get/destroy} through a module_map.h header?
- [x] Rename module_map_* to map_*
- [x] Drop GNU_SOURCE and pipe2 call, unifying pipe()

### Improvements
- [x] add a module_broadcast function
- [x] map_put to take additional "bool key_strdup" param
- [x] Add a map_has_key function + use it where needed
- [x] Avoid MOD_ASSERT spamming in release mode

### Fixes
- [x] Move call to "destroy()" callback after module and self_t destruction; this way no more libmodule things can be called on that being-destroyed module
- [x] Fix poll loop to manage EINTR/EAGAIN interrupt signals
- [x] Properly check poll_set_data call in module.c
- [x] pkg-config cflags should use as includdir /usr/include and not /usr/include/module
- [x] in map_rehash avoid changing needs_free value

### Tests
- [x] Add module_broadcast test
- [x] Add test_module_map.h

### Doc
- [x] Add module_map API documentation

### Porting
- [x] Add support for libkqueue -> this will let libmodule build on solaris and windows too

- [x] Update readme (multi platform + libkqueue mention)

- [ ] Release

## 3.1.1

### Improvements
- [ ] self_t will hold a pointer to module and to its context
- [ ] Store module name in module struct (strdup'd)
- [ ] Store context name in m_context (strdup'd)
- [ ] Use them as hashmap key without strudpping again
- [ ] insert self_t into context->modules, instead of module
- [ ] this way GET_MOD etc etc are way faster (no context and modules hashmap lookup)

- [ ] Release

## 3.2.0
- [ ] Actually implement a queue for module_become/unbecome
- [ ] Expose queue through a queue.h public header
- [ ] Expose a list.h too? And use it for module's fds?

## 4.0.0 (?)
- [ ] Prevent other modules from using a module's self_t (as received eg from a PubSub message)
Idea: for each self_t passed as first parameter, check that its address is same as self_t stored in module (?), eg: 
    module_register_fd(self_t **self, ...) { if (self != (*self)->module->trusted_self) { return MOD_NO_PERM; }

- [ ] All API should go through self_t, eg: self_t *recipient = module_ref("foo"); module_tell(self, recipient, ...);
- [ ] Add a module_new_msg(.data=..., .topic=..., ) (https://github.com/mcinglis/c-style#use-structs-to-name-functions-optional-arguments)

## Ideas

- [ ] Let contexts talk together? Eg: broadcast(msg, bool global) to send a message to all modules in every context; module_publish message in another context? etc etc

### Submodules (??)

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children
