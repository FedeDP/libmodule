#pragma once

#include "module.h"

/* Interface Macros */
#define self() *(get_self())

#define MODULE_CTX(name, ctx) \
static bool init(void); \
static bool check(void); \
static bool evaluate(void); \
static void receive(const msg_t *const msg, const void *userdata); \
static void destroy(void); \
static inline const self_t **get_self() { static const self_t *_self = NULL; return &_self; } \
static void _ctor3_ constructor(void) { \
    if (check()) { \
        userhook_t hook = { init, evaluate, receive, destroy }; \
        module_register(name, ctx, (self_t **)get_self(), &hook, 0); \
    } \
} \
static void _dtor1_ destructor(void) { module_deregister((self_t **)&self()); } \
static void _ctor2_ module_pre_start(void)

#define MODULE(name) MODULE_CTX(name, MODULES_DEFAULT_CTX)

/* Defines for easy API (with no need bothering with both _self and ctx) */
#define m_is(state)                             module_is(self(), state)
#define m_dump()                                module_dump(self())
#define m_stats(stats)                          module_stats(self(), stats);

#define m_start()                               module_start(self())
#define m_pause()                               module_pause(self())
#define m_resume()                              module_resume(self())
#define m_stop()                                module_stop(self())

#define m_set_userdata(userdata)                module_set_userdata(self(), userdata)
#define m_get_userdata()                        module_get_userdata(self())

#define m_log(...)                              module_log(self(), ##__VA_ARGS__)

#define m_name()                                module_name(self());
#define m_ctx()                                 module_ctx(self());
#define m_ref(name, modref)                     module_ref(self(), name, modref)

#define m_become(x)                             module_become(self(), receive_##x)
#define m_unbecome()                            module_unbecome(self())

/* Generic event source registering functions */
#define m_register_src(X, flags, userptr)       module_register_src(self(), X, flags, userptr)
#define m_deregister_src(X)                     module_deregister_src(self(), X)

#define m_tell(recipient, msg, size, flags)     module_tell(self(), recipient, msg, size, flags)
#define m_publish(topic, msg, size, flags)      module_publish(self(), topic, msg, size, flags)
#define m_broadcast(msg, size, flags)           module_broadcast(self(), msg, size, flags)
#define m_poisonpill(recipient)                 module_poisonpill(self(), recipient)
#define m_tell_str(recipient, msg, flags)       module_tell(self(), recipient, (const void *)msg, strlen(msg), flags)
#define m_publish_str(topic, msg, flags)        module_publish(self(), topic, (const void *)msg, strlen(msg), flags)
#define m_broadcast_str(msg, flags)             module_broadcast(self(), (const void *)msg, strlen(msg), flags)

#define m_ps_msg_ref(msg)                       module_ps_msg_ref(self(), msg);
#define m_ps_msg_unref(msg)                     module_ps_msg_ref(self(), msg);
