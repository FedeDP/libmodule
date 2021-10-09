#include <module/plugin_C.h>
#include <string.h>

/* This is a small plugin needed by Easy example, runtime linked */

M_PLUGIN("Test");

bool m_plugin_on_start(m_mod_t *mod) {
    m_mod_src_register(mod, "leaving", 0, NULL);
    m_mod_log(mod, "Linked.\n");
    return true;
}

bool m_plugin_on_eval(m_mod_t *mod) {
    return true;
}

void m_plugin_on_stop(m_mod_t *mod) {

}

void m_plugin_on_evt(m_mod_t *mod, const m_evt_t *msg) {
    if (msg->type == M_SRC_TYPE_PS && msg->ps_evt->type == M_PS_USER) {
        if (!strcmp((char *)msg->ps_evt->data, "ByeBye")) {
            m_mod_log(mod, "Received quit.\n");
        }
    }
}
