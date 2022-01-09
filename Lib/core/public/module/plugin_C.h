#pragma once

#include <module/plugin.h>
#include <module/mod.h>

/** 
 * This is a small C define to ease the use of C plugins.
 **/

#define M_PLUGIN(name) \
    const char m_plugin_name[] = name; \
    bool m_plugin_on_eval(m_mod_t *self); \
    bool m_plugin_on_start(m_mod_t *self); \
    void m_plugin_on_evt(m_mod_t *self, const m_evt_t *const msg); \
    void m_plugin_on_stop(m_mod_t *self);
