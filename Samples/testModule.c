#include <module/mod_easy.h>
#include <string.h>

/** This is a test module needed for Easy example, to runtime link another module **/

M_MOD("Test");

static void m_mod_on_boot(void) {
}

static bool m_mod_on_start(m_mod_t *mod) {
    m_mod_src_register(mod, "leaving", 0, NULL);
    m_mod_log(mod, "Linked.\n");
    return true;
}

static bool m_mod_on_eval(m_mod_t *mod) {
    return true;
}

static void m_mod_on_stop(m_mod_t *mod) {

}

static void m_mod_on_evt(m_mod_t *mod, const m_evt_t *msg) {
    if (msg->type == M_SRC_TYPE_PS && msg->ps_evt->type == M_PS_USER) {
        if (!strcmp((char *)msg->ps_evt->data, "ByeBye")) {
            m_mod_log(mod, "Received quit.\n");
        }
    }
}
