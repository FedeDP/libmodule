# TODO

## Drop on error callback?

- [ ] drop modules_ctx_on_error (easy)
- [ ] expand error defines (eg MOD_NOT_FOUND -2 etc etc) (easy)

## Generic

- [ ] Add a module_pre_start function for each module (easy)
- [X] After module_stop -> STOPPED status (to prevent it being evaluated again after new event)
- [ ] Add a m_update_fd(int fd) function to change fd (easy)

## Much more actor like?

- [ ] implement tell and publish (per-ctx) (easy)
- [ ] implement subscribe (mid)
- [ ] add a (void *) in message_t (easy)
- [ ] implement broadcast (global message, no ctx) (easy)

### UPDATE DOC with changes until there

## Submodules

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)

### UPDATE DOC with SUBMODULE interface

## Test it

- [ ] Check libmodule with valgrind again (last time it had no memleaks)
- [ ] write some tests (cmocka?)
