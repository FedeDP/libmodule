#include <module/mod_easy.h>
#include <module/ctx.h> 
#include <string.h>
#include <stdlib.h>

static void m_mod_on_evt_sleeping(m_mod_t *mod, const m_queue_t *const evts);
static m_mod_t *plugin;

M_MOD("Doggo");

static void m_mod_on_boot(void) {
    printf("Press 'c' to start playing with your own doggo...\n");
}

#ifdef M_CTX_HAS_FS
void m_ctx_pre_loop(m_ctx_t *c, int argc, char *argv[]) {
    m_ctx_fs_set_root(c, "./EasyCtx");
}
#endif

static bool m_mod_on_start(m_mod_t *mod) {
    /* Doggo should subscribe to "leaving" topic, as regex */
    m_mod_src_register(mod, "leav[i+]ng", 0, NULL);
    /* Subscribe to MOD_STOPPED system messages too! */
    m_mod_src_register(mod, M_PS_MOD_STOPPED, 0, NULL);
    return true;
}

static bool m_mod_on_eval(m_mod_t *mod) {
    return true;
}

static void m_mod_on_stop(m_mod_t *mod) {
    if (plugin) {
        m_mem_unrefp((void **)&plugin);
    }
}

static void m_mod_on_evt(m_mod_t *mod, const m_queue_t *const evts) {
    m_itr_foreach(evts, {
        m_evt_t *msg = m_itr_get(m_itr);
        if (msg->type == M_SRC_TYPE_PS) {
            if (msg->ps_evt->topic && strcmp(msg->ps_evt->topic, M_PS_MOD_STOPPED) == 0) {
                if (msg->ps_evt->sender) {
                    const char *name = m_mod_name(msg->ps_evt->sender);
                    m_mod_log(mod, "Module '%s' has been stopped.\n", name);
                } else {
                    m_mod_log(mod, "A module has been deregistered.\n");
                }
            } else {
                if (!strcmp((char *)msg->ps_evt->data, "ComeHere")) {
                    m_mod_log(mod, "Running...\n");
                    m_mod_ps_tell(mod, msg->ps_evt->sender, "BauBau", 0);
                } else if (!strcmp((char *)msg->ps_evt->data, "LetsPlay")) {
                    m_mod_log(mod, "BauBau BauuBauuu!\n");
                } else if (!strcmp((char *)msg->ps_evt->data, "LetsEat")) {
                    m_mod_log(mod, "Burp!\n");
                } else if (!strcmp((char *)msg->ps_evt->data, "LetsSleep")) {
                    m_mod_become(mod, m_mod_on_evt_sleeping);
                    m_mod_log(mod, "ZzzZzz...\n");
                    m_mod_src_register(mod, M_PS_MOD_STARTED, 0, NULL);

                    /* 
                     * Test runtime module loading; loaded module won't have direct access to CTX.
                     * We own a ref on it! 
                     */
                    m_mod_register("./libtestmod.so", m_mod_ctx(mod), &plugin, NULL, M_MOD_DENY_CTX, NULL);
                } else if (!strcmp((char *)msg->ps_evt->data, "ByeBye")) {
                    m_mod_log(mod, "Sob...\n");
                } else if (!strcmp((char *)msg->ps_evt->data, "WakeUp")) {
                    m_mod_log(mod, "???\n");
                }
            }
        }
    });
}

static void m_mod_on_evt_sleeping(m_mod_t *mod, const m_queue_t *const evts) {
    m_itr_foreach(evts, {
        m_evt_t *msg = m_itr_get(m_itr);
        if (msg->type == M_SRC_TYPE_PS) {
            if (msg->ps_evt->topic && strcmp(msg->ps_evt->topic, M_PS_MOD_STARTED) == 0) {
                /* A new module has been started */
                const char *name = m_mod_name(msg->ps_evt->sender);
                m_mod_log(mod, "Module '%s' has been started.\n", name);
            } else {
                if (!strcmp((char *)msg->ps_evt->data, "WakeUp")) {
                    m_mod_unbecome(mod);
                    m_mod_log(mod, "Yawn...\n");
                    m_ctx_dump(m_mod_ctx(mod));
                    m_mod_deregister(&plugin);
                    m_mod_src_deregister(mod, M_PS_MOD_STARTED);
                } else {
                    m_mod_log(mod, "ZzzZzz...\n");
                }
            }
        }
    });
}
