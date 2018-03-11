# TODO

## Submodules

- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)

## Generic

- [x] avoid assert() and better errors handling

## Document it

- [x] write DOC (readthedocs?)
- [ ] write some tests (cmocka?)
