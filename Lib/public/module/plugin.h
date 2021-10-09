#pragma once

#ifndef LIBMODULE_PLUGIN_H
    #define LIBMODULE_PLUGIN_H
#endif
#include "cmn.h"

/**
 * This is the main plugin API:
 * any source code exposing this API can be used as a plugin for libmodule.
 * A plugin is just a module attached at runtime; 
 * indeed this API is the same as the m_mod_hook_t callbacks struct.
 **/

/* Optional; if not set, plugin name will be basename(path) */
extern const char m_plugin_name[];

/* Optional, if left empty, no eval will be made and plugin will immediately start */
bool m_plugin_on_eval(m_mod_t *self);

/* Optional */
bool m_plugin_on_start(m_mod_t *self);

/* MANDATORY */
void m_plugin_on_evt(m_mod_t *self, const m_evt_t *const msg);

/* Optional */
void m_plugin_on_stop(m_mod_t *self);
