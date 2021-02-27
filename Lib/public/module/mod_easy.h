#pragma once

#include "mod.h"

#define M_M(name) \
    static bool m_m_on_start(void); \
    static bool m_m_on_eval(void); \
    static void m_m_on_evt(const m_evt_t *const msg); \
    static void m_m_on_stop(void); \
    static m_mod_t *m_m_self; \
    static void _ctor3_ m_m_ctor(void) { \
        m_mod_hook_t hook = { m_m_on_start, m_m_on_eval, m_m_on_evt, m_m_on_stop }; \
        m_mod_register(name, &m_m_self, &hook, M_MOD_PERSIST, NULL); \
    } \
    static void _dtor1_ m_m_dtor(void) { m_mod_deregister(&m_m_self); } \
    static void _ctor2_ m_m_pre_start(void)

/* Defines for easy API (with no need bothering with mod handler) */

#define m_m_load(path, flags, ref)                m_mod_load(m_m_self, path, flags, ref)
#define m_m_unload(path)                          m_mod_unload(m_m_self, path)

#define m_m_is(state)                             m_mod_is(m_m_self, state)
#define m_m_dump()                                m_mod_dump(m_m_self)
#define m_m_stats(stats)                          m_mod_stats(m_m_self, stats)

#define m_m_start()                               m_mod_start(m_m_self)
#define m_m_pause()                               m_mod_pause(m_m_self)
#define m_m_resume()                              m_mod_resume(m_m_self)
#define m_m_stop()                                m_mod_stop(m_m_self)

#define m_m_userdata()                            m_mod_userdata(m_m_self)

#define m_m_log(...)                              m_mod_log(m_m_self, ##__VA_ARGS__)

#define m_m_name()                                m_mod_name(m_m_self)

#define m_m_ref(name)                             m_mod_ref(m_m_self, name)

#define m_m_become(x)                             m_mod_become(m_m_self, m_m_on_evt_##x)
#define m_m_unbecome()                            m_mod_unbecome(m_m_self)

/* Generic event source registering functions */
#define m_m_src_register(X, flags, userptr)       m_mod_src_register(m_m_self, X, flags, userptr)
#define m_m_src_deregister(X)                     m_mod_src_deregister(m_m_self, X)

#define m_m_ps_tell(recipient, msg, flags)        m_mod_ps_tell(m_m_self, recipient, msg, flags)
#define m_m_ps_publish(topic, msg, flags)         m_mod_ps_publish(m_m_self, topic, msg, flags)
#define m_m_ps_broadcast(msg, flags)              m_mod_ps_broadcast(m_m_self, msg, flags)
#define m_m_ps_poisonpill(recipient)              m_mod_ps_poisonpill(m_m_self, recipient)
