#pragma once

#include "module.h"

/* Interface Macros */
#define self() *(get_self())

#define M_MOD_FULL(name, ctx, flags) \
    static bool init(void); \
    static bool check(void); \
    static bool eval(void); \
    static void receive(const msg_t *const msg, const void *userdata); \
    static void destroy(void); \
    static inline const self_t **get_self() { static const self_t *_self = NULL; return &_self; } \
    static void _ctor3_ constructor(void) { \
        if (check()) { \
            userhook_t hook = { init, eval, receive, destroy }; \
            m_module_register(name, ctx, (self_t **)get_self(), &hook, flags); \
        } \
    } \
    static void _dtor1_ destructor(void) { m_module_deregister((self_t **)&self()); } \
    static void _ctor2_ module_pre_start(void)

#define M_MOD(name) M_MOD_FULL(name, M_CTX_DEFAULT, 0)

/* Defines for easy API (with no need bothering with both _self and ctx) */
#define m_mod_is(state)                             m_module_is(self(), state)
#define m_mod_dump()                                m_module_dump(self())
#define m_mod_stats(stats)                          m_module_stats(self(), stats);

#define m_mod_start()                               m_module_start(self())
#define m_mod_pause()                               m_module_pause(self())
#define m_mod_resume()                              m_module_resume(self())
#define m_mod_stop()                                m_module_stop(self())

#define m_mod_set_userdata(userdata)                m_module_set_userdata(self(), userdata)
#define m_mod_get_userdata()                        m_module_get_userdata(self())

#define m_mod_log(...)                              m_module_log(self(), ##__VA_ARGS__)

#define m_mod_name()                                m_module_name(self());
#define m_mod_ctx()                                 m_module_ctx(self());
#define m_mod_ref(name, modref)                     m_module_ref(self(), name, modref)

#define m_mod_become(x)                             m_module_become(self(), receive_##x)
#define m_mod_unbecome()                            m_module_unbecome(self())

/* Generic event source registering functions */
#define m_mod_register_src(X, flags, userptr)       m_module_register_src(self(), X, flags, userptr)
#define m_mod_deregister_src(X)                     m_module_deregister_src(self(), X)

#define m_mod_tell(recipient, msg, size, flags)     m_module_tell(self(), recipient, msg, size, flags)
#define m_mod_publish(topic, msg, size, flags)      m_module_publish(self(), topic, msg, size, flags)
#define m_mod_broadcast(msg, size, flags)           m_module_broadcast(self(), msg, size, flags)
#define m_mod_poisonpill(recipient)                 m_module_poisonpill(self(), recipient)
#define m_mod_tell_str(recipient, msg, flags)       m_module_tell(self(), recipient, (const void *)msg, strlen(msg), flags)
#define m_mod_publish_str(topic, msg, flags)        m_module_publish(self(), topic, (const void *)msg, strlen(msg), flags)
#define m_mod_broadcast_str(msg, flags)             m_module_broadcast(self(), (const void *)msg, strlen(msg), flags)
