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
- [x] Use flexible array member, less tricky

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
- [x] Add a build-time warning when using liburing as its support is not yet stable nor complete

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

- [x] Rename TYPE_PT to TYPE_PATH and all its occurrencies
- [x] Support SRC_DUP flags for path string

- [x] Rename module.c::is_sgs_same/etc etc to sgscmp() etc etc

### Libfuse support

- [x] Optional build dep
- [x] Integrate fuse fd into libmodule poll loop
- [x] Read on a file, will module_dump()
- [x] Creating new file will register a new "fuse"-only module
- [x] Deleting a previously created fuse module, will deregister it
- [x] Add an interface to customize fuse fs folder (by default: no fuse fs is created)
- [x] Expose <module/fs.h> with FS ioctls
- [x] Put m_ctx_set_fs_root() declaration in module/fs.h 
- [x] Put m_ctx_set_fs_root() definition in fs_priv.c
- [x] Only install fs.h if built WITH_FS
- [x] Notify with poll callback when fuse-module has new receive() events enqueued
- [x] Test poll
- [x] Let non-fuse modules notify poll too? (ie: every module should call fs_recv... may be renamed to fs_notify())
- [x] Add an ioctl interface
- [x] Fix tests
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
- - [x] Rename mod_t and ctx_t
- - [x] Rename msg_t to m_evt_t
- - [x] Rename msg_t sub msgs
- - [x] Rename m_mod_register_src() to m_mod_src_register() ??
- - [x] Rename src register types (eg: mod_tmr_t -> m_tmr_t)
- - [x] Rename userhook_t, log_cb
- - [ ] Rename init_cb and other callbacks
- - [x] Rename memhook_t and its callbacks
- - [x] Rename stats_t to m_stats_t
- - [x] Rename exposed flags
- - [ ] Rename init/deinit/check etc etc to on_start(), on_stop(), on_event(), on_eval() etc etc...
- - [x] Rename commons.h.in to cmn.h.in
- - [x] Rename mod_states to m_mod_states
- [x] Rename pubsub.c to ps.c
- [x] Rename MODULE() to M_MOD()
- [x] Rename MODULE_CTX() to M_MOD_FULL() and take additional flags parameter
- [x] Rename module_cmn.h to commons.h
- [x] Rename complex API to m_mod/ctx_ -> "Easy" API instead "m_m_" and "m_c_"
- [x] Rename evaluate() to eval()
- [x] Rename map/stack/... APIs to m_x_y
- [x] Rename module.* to mod.* and context.* to ctx.*
- [x] Rename test_module to test_mod; rename all module tests to mod
- [x] Rename test_context to test_ctx; rename all context tests to ctx
- [x] Split m_mem API from utils.c to its own source file
- [x] Rename utils.c into priv.c
- [x] Put thpool.c and mem.c into a new "utils" folder

### New ctx_register API

- [x] Add m_ctx_(de)register() API
- [x] Split mod_flags into m_mod_flags and m_ctx_flags
- [x] Easy APIs will only work for single (M_CTX_DEFAULT) context
- [x] Cleanup weak main
- [x] Cleanup M_MOD_FULL (drop ctxname)
- [x] Cleanup M_CTX_FULL (drop ctxname)
- [x] Fix tests
- [x] Fix samples

- [x] Expose a ctx handler (similar to self_t) to avoid expensive map lookup (with global mutex lock)?
- [x] Drop m_mod_ctxname and add m_mod_ctx (that returns ctx handler for mod context)
- [x] Add m_ctx_name() to return name of ctx
- [x] Mod should have a ctx pointer
- [x] Drop self_t and just use mod_t pointer to incomplete type?
- [x] Add new type m_mod_ref_t that is used as recipient for mod_tell() and returned by m_mod_ref()
- [x] Drop module reference concept entirely; it is up to the developer to not misuse libmodule
- [x] Internally, struct _mod_ref will just wrap struct _mod
- [x] Fix FIXME around code
- [x] m_mod_ref() to actually m_mem_ref returned mod
- [x] Add m_mod_unref()
- [x] Update examples adding m_mod_unref in module dtor

- [x] Cleanup priv.h macros?
- [x] Rename ASSERT macros as M_CTX_ASSERT, M_MOD_ASSERT, M_ASSERT, M_RET_ASSERT etc etc,,,
- [x] Add a MOD_CTX_ASSERT(c) that calls MOD_TH_ASSERT too!

- [x] Add m_ctx_set/get_userdata
- [x] Pass context userdata as data for PS system messages

- [x] Update tests
- [x] Update samples
- [x] Updated README example

### New Linked list API

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
- [x] Improve tests
- [x] Fix bst_itr_remove(9)
- [x] Rename tests to test_bst.c

### New iterator APIs

- [x] Add a common m_itr_* API that allows to easily loop on any iterator
- [x] Use it inside various datastructures tests
- [x] Use it where needed
- [x] Implement itr_remove for stack and queue
- [x] Expose itr_remove in itr API
- [x] Expose itr_set in itr API (mbtree excluded)
- [x] change itr_next() to return int and take double ptr

### New thpool API

- [x] Add a new TYPE_TASK source that will run a task asynchronously and at end notify user through receive() callback
- [x] m_module_deregister_task() should pthread_cancel() the thread
- [x] fix m_module_deregister_task -> ALWAYS CREATE THREADS DETACHED

- [x] Add a th_pool implementation, public API
- [x] Allow to pass m_thpool_t in mod_task_t structure; if !NULL, simply schedule the job on the pool
- [x] add thpool example
- [x] Drop max_tasks param in m_thpool_new: users can use m_thpool_length() before calling m_thpool_add()
- [x] Drop bool wait_all flag in m_thpool_wait; by default, any task is waited. If user wish to wait only current tasks, they should call m_thpool_clear() before

### DOC

- [ ] rewrite from scratch!
- [ ] make it explicit in doc that m_set_memhook() should be called within m_pre_start() function
- [ ] Fully rewrite documentation per-namespace
- [ ] Add build options doc
- [ ] Auto-generate API doc, using eg: https://github.com/jnikula/hawkmoth or with doxygen (and drop rtd) https://goseeky.wordpress.com/2017/07/22/documentation-101-doxygen-with-github-pages/

### Remaining fixes/Improvements

- [ ] Expose recv_msg/sent_msg in stats_t and use them as threshold too in m_ctx_trim
- [ ] Fix fetch_ms()! Only count actions as actions triggered by API call (ie: user called an API function on the module)

- [x] Improve init/destroy: if init() gets called at each module (re)start() we need a counterpart, eg: deinit(), called at each stop().
- [x] Then drop destroy() API, useless

- [x] Add M_MOD_USERDATA_AUTOFREE field to automatically free userdata at module deregister
- [x] Add a M_CTX_USERDATA_AUTOFREE field to automatically free context userdata at context deregister
- [x] Allow to pass an userdata pointer in module_register and ctx_register()

- [x] Add support for priority based subscribe? When publishing then create a list of recipients from highest priority to lower, then for each element in the list tell it the message

- [x] m_ctx_load() should take a m_mod_flags parameter to enforce certain flags over runtime loaded module

- [x] Expose mod/ctx -> flags and ctx->userdata in mod/ctx_dump()

- [ ] Add a message compact time, eg: m_mod_set_compact_time(timerspec); then messages are kept on hold for timerspec time before being flushed to module
- [ ] during compaction time, duplicated messages are erased

- [ ] Add a m_mod_set_batch_size(size) to batch requests and flush them together (receive callback will receive pointer to array and length of array)?

- [x] Double check ctx_easy and mod_easy api
- [x] Add -DNO_CHECKS to disable checks

- [x] Avoid direct inclusion of cmn.h

- [ ] Add a module_stash/unstash (all) API for PS messaging? Each module has a queue and ps messages are enqueued; only for msg->type != FD_MSG!

- [x] Put src_userdata inside m_evt_t struct and avoid passing it as param to recv_cb
- [ ] switch m_evt_t to be mem ref'd. This way clients can keep a ref by calling m_mem_ref() on message
- [ ] document behaviour 

- [x] drop check() callback on easy API
- [ ] add m_mod_ prefix to callbacks, eg m_mod_prestart, m_mod_on_start() ecc ecc
- [ ] Use M_M() macro instead of M_MOD? More coherent with easy API; eg: M_MOD(name, ctx) -> M_M(Name) (use null as ctx)
- [ ] use m_m_ prefix for callbacks?

- [x] move m_ctx_load under mod namespace, ie m_mod_load(). 
- [x] In general, follow the following idea: "what are we {loading,dumping, registering...}?" A module. Thus mod namespace

### Module permissions management
- [ ] Add some permission management to modules, through m_mod_flags, eg:
- - [ ] M_MOD_DENY_PUB
- - [ ] M_MOD_DENY_SUB
- - [ ] M_MOD_DENY_LOAD
- - [ ] M_MOD_DENY_CTX (module cannot access its ctx)
- [ ] m_mod_register() add flags
- [ ] m_mod_load() add flags

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
- [x] Fix: actually check that modules_prestart did not modify memhook before overriding it in libmodule_init()

- [x] Only allow to call libmodule API from same thread in which modules/contexts were registered [...]

- [x] MAP API: drop bool dupkeys and add m_map_flags flags!

- [x] Drop "size" from ps_msg_t?
- [x] Drop PS_DUP_DATA flag
- [x] Drop m_m_{tell, publish, broadcast}_str APIs

- [x] Create new pubsub message for each recipient instead of using only one. 
- [x] Drop Queue from pubsub messages

- [x] Create new "main.c" file with libmodule_init(), libmodule_end(), weak main() and m_pre_start()
- [x] Move m_set_memhook in main.c
- [x] do not lock any mutex

- [x] Update libmodule.pc.in to add extra dependencies if needed (libkqueue/liburing/fuse)
- [x] Update examples
- [x] Update tests

- [x] Use unlikely() macro for M_MOD_ASSERT

## 6.1.0 (7.0.0?)

### Map API

- [ ] Allow devs to customize hash function and take a void* as key

## Ideas

- [ ] Add a new module type that only acts as "router" between contexts? eg: ROUTER(name, ctxA, ctxB) 
-> any message received from ctxA will be published in ctxB, else any message received on ctxB will be published in ctxA. eg module_add_route(self, ctxStart, ctxEnd) ?

### Submodules
- [ ] SUBMODULE(B, A) calls module_register(B) and module_binds_to(A);
- [ ] SUBMODULE SHOULD BE STARTED later (after all MODULES) -> ctor3 (this way it'll support only level 1 submodules though (ie: a submodule cannot have submodules))
- [ ] destroy children of modules at module deregister
- [ ] bind children to parent states (ie: parent paused -> children paused; parent resumed -> children resumed...)
- [ ] m_forward -> like m_tell but to all children

### Generic
- [ ] Akka-persistence like message store? (ie: store all messages and replay them)
