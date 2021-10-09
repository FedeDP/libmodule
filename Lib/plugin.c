#include "priv.h"

/** Private API **/

int open_dl_handle(m_ctx_t *c, const char *module_path, m_mod_t **ref, m_mod_flags flags) {
    /* Set the only allowed ctx for m_mod_register() to passed ctx */
    void *handle = dlopen(module_path, RTLD_NOW);
    if (!handle) {
        M_DEBUG("Dlopen failed with error: %s\n", dlerror());
        return -EINVAL;
    }
    
    /* Search for required symbols */
    m_mod_hook_t hook = {0};
    hook.on_eval = dlsym(handle, "m_plugin_on_eval");
    hook.on_start = dlsym(handle, "m_plugin_on_start");
    hook.on_stop = dlsym(handle, "m_plugin_on_stop");
    hook.on_evt = dlsym(handle, "m_plugin_on_evt");
    const char *plugin_name = dlsym(handle, "m_plugin_name");
    
    if (!hook.on_evt) {
        M_DEBUG("Mandatory m_plugin_on_evt() symbol missing.\n");
        return -EBADF;
    }
    
    if (!plugin_name) {
        plugin_name = basename(module_path);
        flags |= M_MOD_NAME_DUP;
    }
    
    m_mod_t *new_mod = NULL;
    int ret = m_mod_register(plugin_name, c, &new_mod, &hook, flags, NULL);
    if (ret == 0) {
        new_mod->dlhandle = handle;
        if (ref) {
            *ref = new_mod;
        } else {
            // Useless reference
            m_mem_unref(new_mod);
        }
    }
    return ret;
}

/** Public API **/

/* Only constant flags are kept */
_public_ int m_plugin_load(m_ctx_t *c, const char *module_path, m_mod_flags flags, m_mod_t **ref) {
    M_PARAM_ASSERT(str_not_empty(module_path));
    
    return open_dl_handle(c, module_path, ref, flags);
}

_public_ int m_plugin_unload(m_ctx_t *c, const char *module_path) {
    M_PARAM_ASSERT(str_not_empty(module_path));
    
    void *handle = dlopen(module_path, RTLD_NOLOAD | RTLD_LAZY);
    if (!handle) {
        return -EINVAL;
    }
    
    m_mod_t *target_mod = NULL;
    /* Check if desired module is actually loaded in mod's context */
    m_itr_foreach(c->modules, {
        m_mod_t *m = m_itr_get(itr);
        if (m->dlhandle && handle == m->dlhandle) {
            target_mod = m;
            memhook._free(itr);
            break;
        }
    });
    
    if (target_mod) {
        return m_mod_deregister(&target_mod);
    }
    M_DEBUG("Module loaded from '%s' not found in ctx '%s'.\n", module_path, c->name);
    return -ENODEV;
}
