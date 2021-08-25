#pragma once

#include "mod.h"

/* Simple macro to automatically manage module lifecycle and callbacks */
#define M_MOD(name) \
    static bool m_mod_on_start(m_mod_t *mod); \
    static bool m_mod_on_eval(m_mod_t *mod); \
    static void m_mod_on_evt(m_mod_t *mod, const m_evt_t *const msg); \
    static void m_mod_on_stop(m_mod_t *mod); \
    static void _m_ctor3_ m_m_ctor(void) { \
        static m_mod_t *m_mod_self; \
        m_mod_hook_t hook = { m_mod_on_start, m_mod_on_eval, m_mod_on_evt, m_mod_on_stop }; \
        m_mod_register(name, NULL, &m_mod_self, &hook, M_MOD_PERSIST | M_MOD_BIND_LOOPING_CTX, NULL); \
    } \
    static void _m_ctor2_ m_mod_pre_start(void)

