## 6.0.0

### Generic

- [x] Finally avoid injecting _self into file-global variables

- [x] Add a module_get_userdata function

- [x] Remove userdata parameter to log_cb

- [x] Add new userptr parameter to module_subscribe (just like module_register_fd())
- [x] Use this new parameter -> pass both fd_msg_t and ps_msg_t "userptr" as receive() userdata parameter (and drop it from ps_msg_t and fd_msg_t)

- [x] Rename Module_subscribe and module_register_fd to module_register_source(type), _Generic.
Ie: if first parameter is an int then call module_register_fd, else if const char*, module register subscribe.
Signature: Module_register_source(int/char*, uint flags, void userptr) -> Flags: FD_AUTOCLOSE and others?

- [x] Drop C++ support (useless...)

- [x] module_message_ref()/ module_message_unref() 
- [ ] remove autofree parameter to publish/broadcast and do not unref anything unless module_message_unref is called! (then, if message reaches 0 ref count, free it) (?)
-> what happens if nobody receives a message? Should it be destroyed?
-> what happends when flushing messages upon module destroy?

- [x] Actually check userhook: at least init() and receive() must be defined
- [x] Let users avoid passing other callbacks

- [x] module_get_name/ctx to return const char *

- [x] enum module_states and enum msg_type to full types!

- [x] Add linked list implementation
- [x] Use it for fds
- [x] Add linked list tests
- [x] Add linked list doc

- [x] modules_dump() (same as module_dump but for modules context!)
- [ ] DOCument module_dump and modules_dump odd letters!!

- [x] In epoll interface, check that events & EPOLLERR is false

- [ ] Fix module_load/unload() ... -> Rename to modules_load/unload()?

- [x] Rename fd_priv_t/ps_priv_t/ps_sub_t? More meaningful names!!

- [ ] Update examples
- [ ] Update tests
- [ ] Update DOC!

## Ideas

### Submodules
- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

### Generic
- [ ] Akka-persistence like message store? (ie: store all messages and replay them)
