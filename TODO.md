## 6.0.0

### liburing

- [x] Introduce an uring poll plugin
- [x] Fix tests with uring
- [x] Add a poll plugin perf test: eg: for (int i = 0; i < 1000; i++) { m_tell(self(), &x); } -> check recv_callback() time!
- [x] Let user build kqueue userspace support on linux too
- [x] linux CI build with uring/kqueue too!
- [x] Add deb/rpm packages deps (liburing/libkqueue)
- [x] Drop poll_new_data/poll_free_data and just alloc/free in ADD/RM poll_set_new_evt() ?
- [x] Avoid initing uring during poll_create() -> what if module is manually started? ring won't yet be created but we would try to register new fds causing a crash...
- [x] Fix failing tests
- [x] Fix valgrind-check tests? "WARNING: unhandled amd64-linux syscall: 425" not much we can do...
- [x] Batch-recv cqes from io_uring_wait_cqe_timeout

### New Sources support

- [x] Rename Module_subscribe and module_register_fd to module_register_src(type), _Generic.
Ie: if first parameter is an int then call module_register_fd, else if const char*, module register subscribe.
Signature: Module_register_src(int/char*, uint flags, void userptr) -> Flags: FD_AUTOCLOSE and others?

- [x] Add module_register_timer()? module_register_signal()? (and add them to m_register_src(struct itimerspec *it/sigset_t *mask))
- [x] Implement register_timer/signal for epoll
- [x] Implement register_timer/signal for uring
- [x] Implement register_timer/signal for kqueue
- [x] Design a correct internal API to provide these functions
- [x] Fix build without __GNU_SOURCE
- [x] module_register_timer should get timerid as input (ie: CLOCK_MONOTONIC etc etc)
- [x] Fix fd_priv_dtor() function
- [x] Fix manage_fds deregister_fd
- [x] Fix fire_once timers in kqueue
- [x] Fix fire_once timers in epoll
- [x] Fix fire_once timers in uring
- [x] Set TYPE_PS on pubsub fd! And avoid all checks (if fd == pubsub_fd[0])
- [x] Fix module_dump
- [x] Check TYPE_PS ...only pubsub_fd[0] should use it in mod->srcs
- [x] Drop is_pubsub field
- [x] Fix tests
- [x] Fix fd closing: timerfd/signalfd should be always closed on RM. In fd_priv_dtor we should only close FD_AUTOCLOSE fds
- [x] Add a FD_AUTOFREE flag to mimic PS_AUTOFREE flag?
- [ ] Port examples
- [x] Rename SRC_RUNONCE to SRC_ONESHOT
- [x] Manage SRC_ONESHOT for subscriptions too
- [x] liburing does not work with signalfd :( (issue: https://github.com/axboe/liburing/issues/5). Seems like kernel-side is moving towards a fix!
- [x] fix libkqueue tests
- [x] Rename module_(un)subscribe to module_(de)register_sub
- [x] Rename x_timer_t to x_tmr_t
- [x] Implement a inotify mechanism (kqueue: EVFILT_VNODE)
- [ ] Implement a proc watcher (kqueue: EVFILT_PROC) (linux: https://lwn.net/Articles/794707/ (http://man7.org/linux/man-pages/man2/pidfd_open.2.html))

- [ ] Abstact away internal fd closing mechanism from poll_plugins (ie: FDs used for inotify watcher, timerfd etc etc that are always closed on poll_set_new_evt() with RM)

- [ ] Expose needed flags (eg: IN_CREATE for linux inotify) as mod_src_flags sub-flags, eg:
SRC_PT_OPEN = 1 << 6 SRC_PT_MOVE = 2 << 6 ???
- [ ] Rename SRC_AUTOCLOSE to SRC_FD_AUTOCLOSE ?

- [x] Use TFD_CLOEXEC flag on timerfd on linux
- [ ] Automatically detect absolute timers on linux? eg:
struct timespec spec;
clock_gettime(tmp->tm_src.its.clock_id, &spec);
long curr_ms = spec.tv_nsec / 1000 / 1000 + spec.tv_sec * 1000;
const int abs_fl = tmp->tm_src.its.ms > curr_ms ? TFD_TIMER_ABSTIME : 0;
tmp->tm_src.f.fd = timerfd_create(tmp->tm_src.its.clock_id, TFD_NONBLOCK | TFD_CLOEXEC | abs_fl);

- [ ] Add a common.c in poll_plugins?
- [ ] Add a common_lin.c in poll_plugins (to manage timerfd/inotify/signalfd)

### New Linked list api

- [x] Add linked list implementation
- [x] Use it for fds
- [x] Add linked list tests
- [x] Add linked list doc
- [x] List_insert to insert to head (O(1))

### Generic

- [x] Finally avoid injecting _self into file-global variables

- [x] Add a module_get_userdata function

- [x] Remove userdata parameter to log_cb

- [x] Add new userptr parameter to module_subscribe (just like module_register_fd())
- [x] Use this new parameter -> pass both fd_msg_t and ps_msg_t "userptr" as receive() userdata parameter (and drop it from ps_msg_t and fd_msg_t)

- [x] Drop C++ support (useless...)

- [x] module_message_ref()/ module_message_unref() 
- [ ] remove autofree parameter to publish/broadcast and do not unref anything unless module_message_unref is called! (then, if message reaches 0 ref count, free it) (?)
-> what happens if nobody receives a message? Should it be destroyed?
-> what happens when flushing messages upon module destroy?

- [x] Actually check userhook: at least init() and receive() must be defined
- [x] Let users avoid passing other callbacks
- [x] init() to return a bool -> if false -> init failed; automatically stop module
- [x] Test new init() behaviour
- [x] Call init() callback every time module is restarted (both from idle and stopped states)
- [x] Destroy any module subscriptions too while stopping a module
- [x] Destroy any stacked recv callback while stopping a module

- [x] module_get_name/ctx to return const char *

- [x] enum module_states and enum msg_type to full types!

- [x] modules_dump() (same as module_dump but for modules context!)
- [x] DOCument module_dump and modules_dump odd letters!!

- [x] In epoll interface, check that events & EPOLLERR is false

- [x] Fix module_load/unload() ... -> Rename to modules_load/unload()?
- [x] modules_unload() should avoid unloading modules in other context
- [x] Fix doc

- [x] Rename fd_priv_t/ps_priv_t/ps_sub_t? More meaningful names!!

- [x] Fix: avoid name clash between stack_t and /usr/include/bits/types/stack_t.h when __GNU_SOURCE is defined
- [x] Similarly, fix list_t, map_t and queue_t... Something like mod_map_t, mod_list_t, mod_queue_t and mod_stack_t

- [x] Rename module_states to mod_states
- [x] Rename module_source_flags to mod_src_flags
- [x] Rename module_ret_code to mod_ret
- [x] Rename stack_ret_code, map_ret_code, etc etc to mod_x_ret

- [x] module_is should return false if mod is NULL

- [ ] Add a module_stash/unstash (all) API? Each module has a queue and messages are ref'd and pushed on queue

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
