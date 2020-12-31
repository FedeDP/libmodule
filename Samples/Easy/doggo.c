#include <module/mod_easy.h> 
#include <module/ctx.h> 
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static void receive_sleeping(const m_evt_t *msg, const void *userdata);

static const m_mod_t *new_mod;

M_MOD("Doggo");

static void m_mod_pre_start(void) {
    printf("Press 'c' to start playing with your own doggo...\n");
}

static bool init(void) {
    /* Doggo should subscribe to "leaving" topic, as regex */
    m_m_src_register("leav[i+]ng", 0, NULL);
    return true;
}

static bool check(void) {
    return true;
}

static bool eval(void) {
    return true;
}

static void deinit(void) {
    
}

/*
 * Default poll callback
 */
static void receive(const m_evt_t *msg, const void *userdata) {
    if (msg->type == M_SRC_TYPE_PS) {
        switch (msg->ps_msg->type) {
        case USER:
            if (!strcmp((char *)msg->ps_msg->data, "ComeHere")) {
                m_m_log("Running...\n");
                m_m_ps_tell(msg->ps_msg->sender, "BauBau", 0);
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsPlay")) {
                m_m_log("BauBau BauuBauuu!\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsEat")) {
                m_m_log("Burp!\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsSleep")) {
                m_m_become(sleeping);
                m_m_log("ZzzZzz...\n");
                
                /* Test runtime module loading */
                m_ctx_load(m_m_ctx(), "./libtestmod.so", 0);
            } else if (!strcmp((char *)msg->ps_msg->data, "ByeBye")) {
                m_m_log("Sob...\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
                m_m_log("???\n");
            }
            break;
        case MODULE_STOPPED: {
                if (msg->ps_msg->sender) {
                    const char *name = m_mod_name(msg->ps_msg->sender);
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

static void receive_sleeping(const m_evt_t *msg, const void *userdata) {
    if (msg->type == M_SRC_TYPE_PS) {
        if (msg->ps_msg->type == USER) {
            if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
                m_m_unbecome();
                m_m_log("Yawn...\n");
                m_m_ps_poisonpill(new_mod);
            } else {
                m_m_log("ZzzZzz...\n");
            }
        } else if (msg->ps_msg->type == MODULE_STARTED) {
            new_mod = msg->ps_msg->sender;
            /* A new module has been started */
            const char *name = m_mod_name(new_mod);
            m_m_log("Module '%s' has been started.\n", name);
        }
    }
}
