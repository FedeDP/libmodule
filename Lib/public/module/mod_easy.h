#pragma once

#include "mod.h"

/* Interface Macros */
#define mod() *(get_mod())

#define M_MOD(name) \
    static bool init(void); \
    static bool eval(void); \
    static void receive(const m_evt_t *const msg); \
    static void deinit(void); \
    static inline m_mod_t **get_mod() { static m_mod_t *_mod = NULL; return &_mod; } \
    static void _ctor4_ m_mod_ctor(void) { \
        m_userhook_t hook = { init, eval, receive, deinit }; \
        m_mod_register(name, NULL, get_mod(), &hook, 0, NULL); \
    } \
    static void _dtor2_ m_mod_dtor(void) { m_mod_deregister(get_mod()); } \
    static void _ctor3_ m_mod_pre_start(void)

/* Defines for easy API (with no need bothering with both _self and ctx) */
#define m_m_ctx()                                 m_mod_ctx(mod())

#define m_m_load(path, flags, ref)                m_mod_load(mod(), path, flags, ref)
#define m_m_unload(path)                          m_mod_unload(mod(), path)

#define m_m_is(state)                             m_mod_is(mod(), state)
#define m_m_dump()                                m_mod_dump(mod())
#define m_m_stats(stats)                          m_mod_stats(mod(), stats)

#define m_m_start()                               m_mod_start(mod())
#define m_m_pause()                               m_mod_pause(mod())
#define m_m_resume()                              m_mod_resume(mod())
#define m_m_stop()                                m_mod_stop(mod())

#define m_m_set_userdata(userdata)                m_mod_set_userdata(mod(), userdata)
#define m_m_get_userdata()                        m_mod_get_userdata(mod())

#define m_m_log(...)                              m_mod_log(mod(), ##__VA_ARGS__)

#define m_m_name()                                m_mod_name(mod())

#define m_m_ref(name, modref)                     m_mod_ref(mod(), name, modref)
#define m_m_unref(modref)                         m_mod_unref(mod(), modref)

#define m_m_become(x)                             m_mod_become(mod(), receive_##x)
#define m_m_unbecome()                            m_mod_unbecome(mod())

/* Generic event source registering functions */
#define m_m_src_register(X, flags, userptr)       m_mod_src_register(mod(), X, flags, userptr)
#define m_m_src_deregister(X)                     m_mod_src_deregister(mod(), X)

#define m_m_ps_tell(recipient, msg, flags)        m_mod_ps_tell(mod(), recipient, msg, flags)
#define m_m_ps_publish(topic, msg, flags)         m_mod_ps_publish(mod(), topic, msg, flags)
#define m_m_ps_broadcast(msg, flags)              m_mod_ps_broadcast(mod(), msg, flags)
#define m_m_ps_poisonpill(recipient)              m_mod_ps_poisonpill(mod(), recipient)
