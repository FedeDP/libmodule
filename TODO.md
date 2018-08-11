## 2.2.0 (possibly 3.0.0)

- [ ] pubsub messaging to send bytes instead of string (ie: add size to pubsub_msg_t)
- [ ] Avoid strdup for pubsub msg and trust user-provided pointer
- [ ] Add a module_tell_dup/module_publish_dup to do a malloc and memcpy -> use this if you want every module to receive unique pointer to memory
- [x] Use memhook malloc/free in epoll_priv, kqueue_priv poll_set_data()
- [x] Avoid strdup and use internal strdup that makes use of memhook
- [ ] If 3.0.0 (api break) drop module_add_fd/rm_fd

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
