## 3.0.0

- [x] pubsub messaging to send bytes instead of string (ie: add size to pubsub_msg_t)
- [x] Avoid strdup for pubsub msg and trust user-provided pointer
- [x] Same as above for topic
- [x] Use memhook malloc/free in epoll_priv, kqueue_priv poll_set_data()
- [x] Avoid strdup and use internal strdup that makes use of memhook
- [x] modules_quit should take an "exit value" parameter
- [x] Modules constructor should have higher priority (eg: start from 110?)
- [x] Add module_tell/publish/reply/broadcast_str macro?

- [x] module_register_fd to take an "autoclose" parameter; drop "close" parameter from module_deregister_fd
- [x] module_register_fd to take an *userptr variable too. The pointer will be then forwarded in receive() when called on that fd

- [x] Avoid memleak in hashmap_rehash function

- [x] Constify as much as possible
- [x] Use stdbool where needed

- [x] Drop module_add_fd/rm_fd alias
- [x] Drop module_update_fd()

- [x] Install license file in /usr/share/licenses/libmodule/

- [x] Add a module_easy.h public header? modules_easy.h too?

### Pre-Release

- [x] Update examples and tests
- [x] Update doc

### Release

- [x] Switch to 3.0.0
- [ ] Roll new release

## Later/Ideas

- [ ] Let contexts talk together? Eg: broadcast(msg, bool global) to send a message to all modules in every context; module_publish message in another context? etc etc

### Submodules (??)

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

- [ ] Release
