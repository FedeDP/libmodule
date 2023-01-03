#pragma once

#include <module/mod.h>
#include <module/ctx.h>
#include <module/mem/mem.h>

#define M_CTX_DEFAULT           "libmodule"

/*
 * ctors order:
 * 0) each m_mod_on_boot() (only mod_easy API)
 * 1) each m_mod_ctor() (only mod_easy API)
 */
#define _m_ctor0_         __attribute__((constructor (111)))
#define _m_ctor1_         __attribute__((constructor (112)))

/* Simple macro to automatically manage module lifecycle and callbacks */
#define M_MOD(name) \
    static bool m_mod_on_start(m_mod_t *mod); \
    static bool m_mod_on_eval(m_mod_t *mod); \
    static void m_mod_on_evt(m_mod_t *mod, const m_queue_t *const evts); \
    static void m_mod_on_stop(m_mod_t *mod); \
    static void _m_ctor1_ m_m_ctor(void) { \
        m_ctx_register(M_CTX_DEFAULT, 0, NULL); \
        m_mod_hook_t hook = { m_mod_on_start, m_mod_on_eval, m_mod_on_evt, m_mod_on_stop }; \
        m_mod_register(name, NULL , &hook, M_MOD_PERSIST, NULL); \
    } \
    static void _m_ctor0_ m_mod_on_boot(void)
