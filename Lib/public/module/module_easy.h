#pragma once

#include "module.h"

/* Interface Macros */
#define self() *(get_self())

#define M_MOD(name) \
    static bool init(void); \
    static bool check(void); \
    static bool eval(void); \
    static void receive(const msg_t *const msg, const void *userdata); \
    static void destroy(void); \
    static inline const self_t **get_self() { static const self_t *_self = NULL; return &_self; } \
    static void _ctor4_ m_mod_ctor(void) { \
        if (check()) { \
            userhook_t hook = { init, eval, receive, destroy }; \
            m_mod_register(name, NULL, (self_t **)get_self(), &hook, 0); \
        } \
    } \
    static void _dtor2_ m_mod_dtor(void) { m_mod_deregister((self_t **)get_self()); } \
    static void _ctor3_ m_mod_pre_start(void)

/* Defines for easy API (with no need bothering with both _self and ctx) */
#define m_m_ctx()                                 m_mod_ctx(self())

#define m_m_is(state)                             m_mod_is(self(), state)
#define m_m_dump()                                m_mod_dump(self())
#define m_m_stats(stats)                          m_mod_stats(self(), stats)

#define m_m_start()                               m_mod_start(self())
#define m_m_pause()                               m_mod_pause(self())
#define m_m_resume()                              m_mod_resume(self())
#define m_m_stop()                                m_mod_stop(self())

#define m_m_set_userdata(userdata)                m_mod_set_userdata(self(), userdata)
#define m_m_get_userdata()                        m_mod_get_userdata(self())

#define m_m_log(...)                              m_mod_log(self(), ##__VA_ARGS__)

#define m_m_name()                                m_mod_name(self())

#define m_m_ref(name, modref)                     m_mod_ref(self(), name, modref)

#define m_m_become(x)                             m_mod_become(self(), receive_##x)
#define m_m_unbecome()                            m_mod_unbecome(self())

/* Generic event source registering functions */
#define m_m_register_src(X, flags, userptr)       m_mod_register_src(self(), X, flags, userptr)
#define m_m_deregister_src(X)                     m_mod_deregister_src(self(), X)

#define m_m_tell(recipient, msg, flags)           m_mod_tell(self(), recipient, msg, flags)
#define m_m_publish(topic, msg, flags)            m_mod_publish(self(), topic, msg, flags)
#define m_m_broadcast(msg, flags)                 m_mod_broadcast(self(), msg, flags)
#define m_m_poisonpill(recipient)                 m_mod_poisonpill(self(), recipient)
