## 6.0.0

### Reference-counted objects' life management

- [x] Keep objects alive as long as someone references them
- [x] Keep module's subscription source alive until all messages on the source are received
- [x] Keep context alive while looping
- [x] Actually leave loop as soon as there are no more modules inside
- [x] Keep module's alive while any message references it as sender
- [x] Fix ref'd FS support
- [x] Use mem_ref for create_pubsub_message too! and drop ps_msg->refs counter
- [x] Fix: fix ps_priv_t->sub mem_ref/unref
- [x] Expose new public header <module/mem.h>?
- [x] Add a MEM_LOCK macro to keep a referenced memory alive
- [x] Fix: let user only use m_module_name()/m_module_ctx()/m_module_is() on module's reference on a deregistered module (kept alive by some PS messaging)

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
- [x] Fix: avoid opening a new internal fd (timerfd, signalfd etc etc) at each new poll_set_new_evt ADD call
- [ ] Fix: liburing ... it does not work with timerfd, signalfd and inotifyfd... (well it works, but weirdly!) -> see https://github.com/axboe/liburing/issues/53

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
- [x] Rename SRC_RUNONCE to SRC_ONESHOT
- [x] Manage SRC_ONESHOT for subscriptions too
- [x] liburing does not work with signalfd :( (issue: https://github.com/axboe/liburing/issues/5). Seems like kernel-side is moving towards a fix!
- [x] fix libkqueue tests
- [x] Rename module_(un)subscribe to module_(de)register_sub
- [x] Rename x_timer_t to x_tmr_t
- [x] Implement a inotify mechanism (kqueue: EVFILT_VNODE)
- [x] Implement a proc watcher (kqueue: EVFILT_PROC) (linux: https://lwn.net/Articles/794707/ (http://man7.org/linux/man-pages/man2/pidfd_open.2.html))
- [x] Rename SRC_AUTOCLOSE to SRC_FD_AUTOCLOSE ?

- [x] Use TFD_CLOEXEC flag on timerfd on linux
- [x] Add a common_lin.c in poll_plugins (to manage timerfd/inotify/signalfd)
- [x] SRC_TMR_ABSOLUTE for absolute timers
- [x] Use SFD_CLOEXEC flag on signalfd on linux

- [x] Only use fd_src_t inside tmr_src_t/sgn_src_t etc etc on linux

- [x] Add a new TYPE_TASK source that will run a task asynchronously and at end notify user through receive() callback
- [x] m_module_deregister_task() should pthread_cancel() the thread
- [ ] Add a th_pool implementation, public API (https://github.com/mbrossard/threadpool/blob/master/src/threadpool.c https://nachtimwald.com/2019/04/12/thread-pool-in-c/)
- [ ] Allow to pass m_thpool_t in mod_task_t structure; if !NULL, simply schedule the job on the pool
- [ ] fix m_module_deregister_task ?

- [x] Rename TYPE_PT to TYPE_PATH and all its occurrencies

- [x] Rename module.c::is_sgs_same/etc etc to sgscmp() etc etc

### Libfuse support

- [x] Optional build dep
- [x] Integrate fuse fd into libmodule poll loop
- [x] Read on a file, will module_dump()
- [x] Creating new file will register a new "fuse"-only module
- [x] Deleting a previously created fuse module, will deregister it
- [ ] Add an interface to customize fuse fs folder
- [ ] by default, use "$HOME/.modules/$ctxname/"
- [x] Notify with poll callback when fuse-module has new receive() events enqueued
- [x] Test poll
- [x] Let non-fuse modules notify poll too? (ie: every module should call fs_recv... may be renamed to fs_notify())
- [x] Add an ioctl interface? https://libfuse.github.io/doxygen/ioctl_8c_source.html eg: MOD_PEEK_MSG, MOD_PUB_MSG, MOD_TELL_MSG, MOD_REG_SRC, MOD_DEREG_SRC, MOD_STATS... 
- [x] Use read() to read messages after poll wake-up: if (mod->has_msg) { write_msg } else { module_dump }
- [x] Fix crashes...
- [x] Fix memleaks (only when using fuse)
- [x] Fix tests
- [ ] Use write() to send messages?? if (mod->sending_data) { read } else return -EPERM
- [x] Allow looping context without running modules (drop c->running_mods)
- [x] Allow to deregister non-fuse modules too on unlink
- [x] Fix FIXME inside fuse_priv.c!
- [x] Avoid continuous lookup to modules map; store in fuse_file_info->fh our module
- [x] When deregistering a module through module_deregister(), notify fuse fs and destroy clients associated with that module if needed...
- [x] enable FS in CI builds + tests
- [x] Fix open callback checks

### API renamings/refactor

- [x] Rename mod_stack_t/mod_map_t etc etc to m_map_t/m_stack_t
- [x] Rename modules_* API to m_context_*
- [x] Rename modules_easy.h API to m_ctx_XXX ?
- [x] Rename modules.{c,h} to context.{c,h} ?
- [x] Rename context.c internal API to remove "modules" naming and use context_ instead
- [x] "m_" -> libmodule prefix
- [x] "mod_" -> module API prefix
- [x] "ctx_" -> context API prefix
- [x] "mem_" -> ref'd mem API prefix
- [x] "map/list/queue/stack_" -> data structures API prefix
- [x] "" (no prefix) -> global API
- [x] Rename module_* API to m_module_*
- [x] Rename module_easy API to m_mod_*
- [x] Rename mem API to m_mem*
- [ ] Rename other exposed APIs (types/enums/enum values) to m_*
- [x] Rename MODULE() to M_MOD()
- [x] Rename MODULE_CTX() to M_MOD_FULL() and take additional flags parameter
- [x] Rename module_cmn.h to commons.h
- [x] Rename complex API to m_mod/ctx_ -> "Easy" API instead "m_m_" and "m_c_"
- [x] Rename evaluate() to eval()
- [x] Rename map/stack/... APIs to m_x_y

- [ ] Add m_ctx_(de)register() API
- [ ] Split mod_flags into m_mod_flags and m_ctx_flags

### New Linked list api

- [x] Add linked list implementation
- [x] Use it for fds
- [x] Add linked list tests
- [x] Add linked list doc
- [x] List_insert to insert to head (O(1))
- [x] Fix bug in list_itr_next() when a node was previously removed (ie: avoid skipping an element, as current element is already "next" after a node deletion)

### New btree API

- [x] Use an array of trees instead of single list? ie: one for each SRC_TYPE. Pro: quicker find, quicker remove; Cons: slower insert.
It would allows to check if same node already exists on insert, without losing too much time
- [x] Implement m_btree_traverse()
- [x] Implement m_btree_itr_t
- [x] Implement tests
- [x] Implement m_btree_itr_remove()
- [x] Implement btree_clear through iterator
- [x] Add test_module.c testing that same source cannot be registered twice (-EEXIST)
- [x] Rename btree to bst

### New iterator APIs

- [x] Add a common m_itr_* API that allows to easily loop on any iterator
- [x] Use it inside various datastructures tests
- [x] Use it where needed
- [x] Implement itr_remove for stack and queue
- [x] Expose itr_remove in itr API
- [x] Expose itr_set in itr API (mbtree excluded)
- [x] change itr_next() to return int and take double ptr

### DOC

- [ ] Fully rewrite documentation per-namespace

### Generic

- [x] Finally avoid injecting _self into file-global variables

- [x] Add a module_get_userdata function

- [x] Store self_t *self inside mod_t, and rename old self_t self as self_t ref
- [x] Store a self_t* inside msg_t object to allow to easily recognize module which received a message when >1 modules share same receive() callback

- [x] Remove userdata parameter from log_cb

- [x] Add new userptr parameter to module_subscribe (just like module_register_fd())
- [x] Use this new parameter -> pass both fd_msg_t and ps_msg_t "userptr" as receive() userdata parameter (and drop it from ps_msg_t and fd_msg_t)

- [x] Drop C++ support (useless...)

- [x] keep some module stats, eg: time_t last_msg ( last recved), and messages per millisecond
- [x] add a modules_trim function to deregister "inactive" modules, ie modules whose values are below user settled thresholds
- [x] Add a stats_t type and use that as parameter to modules_trim?
- [x] Add module_get_stats() API
- [x] Split "recv_msg/sent_msg" from stats.msg_ctr

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
- [x] Json-ify module(s)_dump output?

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

- [x] Add mod_ps_flags -> PS_AUTOFREE, PS_GLOBAL, PS_DUP_DATA and change module_tell/publish/broadcast API?
- [x] Always use registered subscriptions topic when sending a message to a module, instead of trusting user-provided topic parameter in module_publish
- [x] module_msg_ref()/ module_msg_unref() to keep a PubSub message alive (incrementing its ref counter)
- [x] Force PS_AUTOFREE flag if PS_DUP_DATA bit is set

- [x] Rename module getters (w/o setters) to module_X() instead of module_get_X()

- [x] module_register to take additional flags parameter

- [x] Protect internal global variables with mutex

- [x] When deregistering a subscription, if no subs are left, destroy map to free memory
- [x] Drop module_ps_msg_(un)ref API?

- [x] Cleanup some unused internal functions (eg: _module_is())

- [x] Update map, list, stash, queue api to nullify param in free().

- [x] modules_ctx_loop() to return real number of dispatched messages

- [x] Rename "poll_plugins/" folder to "poll/"
- [x] Rename priv.c to utils.c
- [x] Move context creation functions to modules.c

- [x] Added queue_remove API to remove instead of dequeueing head of queue

- [x] Add a MOD_PERSIST module's flag to disallow module_deregister on a module
- [x] Actually call module_deregister() on old module when replacing it (MOD_ALLOW_REPLACE flag)
- [x] Switch to errno for errors? (ie: only define MOD_OK/MOD_ERR return codes and then rely upon errno?)

- [x] Fix modules_ctx_dispatch() to just reutrn number of dispatched messages (or -errno)
- [x] Fix modules_ctx_fd() to just reutrn context's fd (or -errno)
- [ ] Fix fetch_ms()! Only count actions as actions triggered by API call (ie: user called an API function on the module)
- [x] Fix: actually check that modules_prestart did not modify memhook before overriding it in libmodule_init()

- [x] Only allow to call libmodule API from same thread in which modules/contexts were registered [...]

- [x] MAP API: drop bool dupkeys and add m_map_flags flags!

- [ ] Drop "size" from ps_msg_t?

- [x] Create new pubsub message for each recipient instead of using only one. 
- [x] Drop Queue from pubsub messages

- [x] Create new "main.c" file with libmodule_init(), libmodule_end(), weak main() and m_pre_start()

- [x] Update libmodule.pc.in to add extra dependencies if needed (libkqueue/liburing/fuse)
- [ ] Update examples
- [ ] Update tests
- [ ] Update DOC!
- [ ] Add build options doc

## Ideas

- [ ] Add a new module type that only acts as "router" between contexts? eg: ROUTER(name, ctxA, ctxB) 
-> any message received from ctxA will be published in ctxB, else any message received on ctxB will be published in ctxA. eg module_add_route(self, ctxStart, ctxEnd) ?
- [ ] Add a module_stash/unstash (all) API for PS messaging? Each module has a queue and ps messages are enqueued

### Submodules
- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

### Generic
- [ ] Akka-persistence like message store? (ie: store all messages and replay them)
