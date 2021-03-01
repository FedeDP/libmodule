#include <module/mod_easy.h>
#include <module/ctx.h> 
#include <string.h>

static void m_m_on_evt_sleeping(const m_evt_t *msg);

static const m_mod_t *new_mod;

M_M("Doggo");

static void m_m_pre_start(void) {
    printf("Press 'c' to start playing with your own doggo...\n");
}

static bool m_m_on_start(void) {
    /* Doggo should subscribe to "leaving" topic, as regex */
    m_m_src_register("leav[i+]ng", 0, NULL);
    return true;
}

static bool m_m_on_eval(void) {
    return true;
}

static void m_m_on_stop(void) {
    
}

/*
 * Default poll callback
 */
static void m_m_on_evt(const m_evt_t *msg) {
    if (msg->type == M_SRC_TYPE_PS) {
        switch (msg->ps_evt->type) {
        case M_PS_USER:
            if (!strcmp((char *)msg->ps_evt->data, "ComeHere")) {
                m_m_log("Running...\n");
                m_m_ps_tell(msg->ps_evt->sender, "BauBau", 0);
            } else if (!strcmp((char *)msg->ps_evt->data, "LetsPlay")) {
                m_m_log("BauBau BauuBauuu!\n");
            } else if (!strcmp((char *)msg->ps_evt->data, "LetsEat")) {
                m_m_log("Burp!\n");
            } else if (!strcmp((char *)msg->ps_evt->data, "LetsSleep")) {
                m_m_become(sleeping);
                m_m_log("ZzzZzz...\n");
                
                /* Test runtime module loading */
                m_m_load("./libtestmod.so", 0, NULL);
            } else if (!strcmp((char *)msg->ps_evt->data, "ByeBye")) {
                m_m_log("Sob...\n");
            } else if (!strcmp((char *)msg->ps_evt->data, "WakeUp")) {
                m_m_log("???\n");
            }
            break;
        case M_PS_MOD_STOPPED: {
                if (msg->ps_evt->sender) {
                    const char *name = m_mod_name(msg->ps_evt->sender);
                    m_m_log("Module '%s' has been stopped.\n", name);
                } else {
                    m_m_log("A module has been deregistered.\n");
                }
            }
            break;
        default:
            break;
        }
    }
}

static void m_m_on_evt_sleeping(const m_evt_t *msg) {
    if (msg->type == M_SRC_TYPE_PS) {
        if (msg->ps_evt->type == M_PS_USER) {
            if (!strcmp((char *)msg->ps_evt->data, "WakeUp")) {
                m_m_unbecome();
                m_m_log("Yawn...\n");
                m_m_ps_poisonpill(new_mod);
            } else {
                m_m_log("ZzzZzz...\n");
            }
        } else if (msg->ps_evt->type == M_PS_MOD_STARTED) {
            new_mod = msg->ps_evt->sender;
            /* A new module has been started */
            const char *name = m_mod_name(new_mod);
            m_m_log("Module '%s' has been started.\n", name);
        }
    }
}
