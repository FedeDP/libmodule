#include "mod.h"
#include "ctx.h"
#include <dlfcn.h> // dlopen

/*****************************************************************
 * Code related to plugins (shared object runtime attached) API. *
 *****************************************************************/

/** Public API **/

/* Only constant flags are kept */
_public_ int m_plugin_load(const char *plugin_path, m_ctx_t *c, m_mod_t **ref, m_mod_flags flags) {
    M_CTX_ASSERT(c);
    M_PARAM_ASSERT(str_not_empty(plugin_path));
    
    /* Set the only allowed ctx for m_mod_register() to passed ctx */
    void *handle = dlopen(plugin_path, RTLD_NOW);
    if (!handle) {
        M_WARN("Dlopen failed with error: %s\n", dlerror());
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
        M_WARN("Mandatory m_plugin_on_evt() symbol missing.\n");
        return -EBADF;
    }

    if (!plugin_name) {
        M_WARN("Mandatory m_plugin_name symbol missing.\n");
        return -EBADF;
    }
    
    m_mod_t *new_mod = NULL;
    int ret = 0;
    do {
        ret = m_mod_register(plugin_name, c, &new_mod, &hook, flags, NULL);
        if (ret != 0) {
            break;
        }

        new_mod->dlhandle = handle;
        if (ref) {
            *ref = new_mod;
        } else {
            // useless reference
            m_mem_unref(new_mod);
        }
        return 0;
    } while (false);

    /* cleanup code in event of error */
    dlclose(handle);
    if (new_mod) {
        new_mod->dlhandle = NULL;
        m_mod_deregister(&new_mod);
    }
    return ret;
}

_public_ int m_plugin_unload(const char *plugin_path, m_ctx_t *c) {
    M_CTX_ASSERT(c);
    M_PARAM_ASSERT(str_not_empty(plugin_path));
    
    void *handle = dlopen(plugin_path, RTLD_NOW | RTLD_NOLOAD);
    if (!handle) {
        M_WARN("Dlopen failed with error: %s\n", dlerror());
        return -EINVAL;
    }
    
    const char *plugin_name = dlsym(handle, "m_plugin_name");
    if (!plugin_name) {
        M_WARN("Mandatory m_plugin_name symbol missing.\n");
        return -EBADF;
    }
    
    m_mod_t *plugin = m_map_get(c->modules, plugin_name);
    if (plugin) {
        return mod_deregister(&plugin, false);
    }
    /*
     * This should never happen as RTLD_NOLOAD flag only returns a dlhandle 
     * for objects that are already loaded in the process address space.
     */
    M_DEBUG("Plugin loaded from '%s' not found in ctx '%s'.\n", plugin_path, c->name);
    return -ENODEV;
}
