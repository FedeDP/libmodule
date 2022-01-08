#include "mod.h"
#include "ctx.h"
#include <dlfcn.h> // dlopen
#include <libgen.h>

/*****************************************************************
 * Code related to plugins (shared object runtime attached) API. *
 *****************************************************************/

static void plugin_dtor(void *src);

static void plugin_dtor(void *src) {
    m_mod_t *mod = (m_mod_t *)src;
    dlclose(mod->dlhandle);
    mod->dlhandle = NULL;
    mod->plugin_path = NULL;
}

/** Public API **/

/* Only constant flags are kept */
_public_ int m_plugin_load(const char *plugin_path, m_ctx_t *c, m_mod_t **ref, m_mod_flags flags) {
    M_CTX_ASSERT(c);
    M_PARAM_ASSERT(str_not_empty(plugin_path));

    if (m_map_contains(c->plugins, plugin_path)) {
        return -EEXIST;
    }
    
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
    
    char *tmp_plugin_path = NULL;
    if (!plugin_name) {
        // NOTE: basename can modify its arg!
        tmp_plugin_path = mem_strdup(plugin_path);
        plugin_name = basename(tmp_plugin_path);
        flags |= M_MOD_NAME_DUP; // dup the name when registering!
    }
    
    m_mod_t *new_mod = NULL;
    int ret = 0;
    do {
        ret = m_mod_register(plugin_name, c, &new_mod, &hook, flags, NULL);
        memhook._free(tmp_plugin_path);
        if (ret != 0) {
            break;
        }

        new_mod->dlhandle = handle;
        new_mod->plugin_path = mem_strdup(plugin_path);

        /* Lazy creation */
        if (!c->plugins) {
            c->plugins = m_map_new(M_MAP_KEY_AUTOFREE, plugin_dtor);
            if (!c->plugins) {
                ret = -ENOMEM;
                break;
            }
        }

        ret = m_map_put(c->plugins, new_mod->plugin_path, new_mod);
        if (ret != 0) {
            break;
        }

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
        memhook._free((void *)new_mod->plugin_path);
        new_mod->plugin_path = NULL;
        new_mod->dlhandle = NULL;
        m_mod_deregister(&new_mod);
    }
    return ret;
}

_public_ int m_plugin_unload(const char *plugin_path, m_ctx_t *c) {
    M_CTX_ASSERT(c);
    M_PARAM_ASSERT(str_not_empty(plugin_path));
    
    m_mod_t *mod = m_map_get(c->plugins, plugin_path);
    if (mod) {
        return mod_deregister(&mod, false);
    }
    M_DEBUG("Module loaded from '%s' not found in ctx '%s'.\n", plugin_path, c->name);
    return -ENODEV;
}
