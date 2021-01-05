#pragma once

#include "mod.h"

#define M_MOD(name, ctx) \
    static bool init(void); \
    static bool eval(void); \
    static void receive(const m_evt_t *const msg); \
    static void deinit(void); \
    static m_mod_t *m_mod; \
    static void _ctor4_ m_mod_ctor(void) { \
        m_userhook_t hook = { init, eval, receive, deinit }; \
        m_mod_register(name, ctx, &m_mod, &hook, 0, NULL); \
    } \
    static void _dtor2_ m_mod_dtor(void) { m_mod_deregister(&m_mod); } \
    static void _ctor3_ m_mod_pre_start(void)

/* Defines for easy API (with no need bothering with mod handler) */
#define m_m_ctx()                                 m_mod_ctx(m_mod)

#define m_m_load(path, flags, ref)                m_mod_load(m_mod, path, flags, ref)
#define m_m_unload(path)                          m_mod_unload(m_mod, path)

#define m_m_is(state)                             m_mod_is(m_mod, state)
#define m_m_dump()                                m_mod_dump(m_mod)
#define m_m_stats(stats)                          m_mod_stats(m_mod, stats)

#define m_m_start()                               m_mod_start(m_mod)
#define m_m_pause()                               m_mod_pause(m_mod)
#define m_m_resume()                              m_mod_resume(m_mod)
#define m_m_stop()                                m_mod_stop(m_mod)

#define m_m_set_userdata(userdata)                m_mod_set_userdata(m_mod, userdata)
#define m_m_get_userdata()                        m_mod_get_userdata(m_mod)

#define m_m_log(...)                              m_mod_log(m_mod, ##__VA_ARGS__)

#define m_m_name()                                m_mod_name(m_mod)

#define m_m_ref(name)                             m_mod_ref(m_mod, name)

#define m_m_become(x)                             m_mod_become(m_mod, receive_##x)
#define m_m_unbecome()                            m_mod_unbecome(m_mod)

/* Generic event source registering functions */
#define m_m_src_register(X, flags, userptr)       m_mod_src_register(m_mod, X, flags, userptr)
#define m_m_src_deregister(X)                     m_mod_src_deregister(m_mod, X)

#define m_m_ps_tell(recipient, msg, flags)        m_mod_ps_tell(m_mod, recipient, msg, flags)
#define m_m_ps_publish(topic, msg, flags)         m_mod_ps_publish(m_mod, topic, msg, flags)
#define m_m_ps_broadcast(msg, flags)              m_mod_ps_broadcast(m_mod, msg, flags)
#define m_m_ps_poisonpill(recipient)              m_mod_ps_poisonpill(m_mod, recipient)
