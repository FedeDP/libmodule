#pragma once

#include <module/mod.h>
#include <module/ctx.h>
#include <module/mem/mem.h>

#define M_CTX_DEFAULT           "libmodule"

/*
 * ctors order:
 * 0) global m_on_boot(), useful to set your own memhook if desired
 * 1) each m_mod_on_boot() (only mod_easy API)
 * 2) each m_mod_ctor() (only mod_easy API)
 *
 * dtors order:
 * 0) each m_mod_dtor() (only mod_easy API)
 */
#define _m_ctor0_         __attribute__((constructor (110)))
#define _m_ctor1_         __attribute__((constructor (113)))
#define _m_ctor2_         __attribute__((constructor (114)))
#define _m_dtor0_         __attribute__((destructor (111)))

/* Simple macro to automatically manage module lifecycle and callbacks */
#define M_MOD(name) \
    static bool m_mod_on_start(m_mod_t *mod); \
    static bool m_mod_on_eval(m_mod_t *mod); \
    static void m_mod_on_evt(m_mod_t *mod, const m_queue_t *const evts); \
    static void m_mod_on_stop(m_mod_t *mod); \
    static m_ctx_t *_m_ctx; \
    static void _m_ctor2_ m_m_ctor(void) { \
        _m_ctx = m_ctx(); \
        if (!_m_ctx) { \
            m_ctx_register(M_CTX_DEFAULT, &_m_ctx, 0, NULL); \
        } \
        m_mod_hook_t hook = { m_mod_on_start, m_mod_on_eval, m_mod_on_evt, m_mod_on_stop }; \
        m_mod_register(name, _m_ctx, NULL , &hook, M_MOD_PERSIST, NULL); \
    } \
    static void _m_dtor0_ m_mod_dtor(void) { \
        m_mem_unrefp((void **)&_m_ctx); \
    } \
    static void _m_ctor1_ m_mod_on_boot(void)
