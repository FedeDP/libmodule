#include <module/module_easy.h> 
#include <module/context_easy.h> 
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static void receive_sleeping(const msg_t *msg, const void *userdata);

static const self_t *new_mod;

MODULE("Doggo");

static void module_pre_start(void) {
    printf("Press 'c' to start playing with your own doggo...\n");
}

static bool init(void) {
    /* Doggo should subscribe to "leaving" topic, as regex */
    m_mod_register_src("leav[i+]ng", 0, NULL);
    return true;
}

static bool check(void) {
    return true;
}

static bool eval(void) {
    return true;
}

static void destroy(void) {
    
}

/*
 * Default poll callback
 */
static void receive(const msg_t *msg, const void *userdata) {
    if (msg->type == TYPE_PS) {
        switch (msg->ps_msg->type) {
        case USER:
            if (!strcmp((char *)msg->ps_msg->data, "ComeHere")) {
                m_mod_log("Running...\n");
                m_mod_tell_str(msg->ps_msg->sender, "BauBau", 0);
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsPlay")) {
                m_mod_log("BauBau BauuBauuu!\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsEat")) {
                m_mod_log("Burp!\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsSleep")) {
                m_mod_become(sleeping);
                m_mod_log("ZzzZzz...\n");
                
                /* Test runtime module loading */
                m_ctx_load("./libtestmod.so");
            } else if (!strcmp((char *)msg->ps_msg->data, "ByeBye")) {
                m_mod_log("Sob...\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
                m_mod_log("???\n");
            }
            break;
        case MODULE_STOPPED: {
                if (msg->ps_msg->sender) {
                    const char *name = m_module_name(msg->ps_msg->sender);
                    m_mod_log("Module '%s' has been stopped.\n", name);
                } else {
                    m_mod_log("A module has been deregistered.\n");
                }
            }
            break;
        default:
            break;
        }
    }
}

static void receive_sleeping(const msg_t *msg, const void *userdata) {
    if (msg->type == TYPE_PS) {
        if (msg->ps_msg->type == USER) {
            if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
                m_mod_unbecome();
                m_mod_log("Yawn...\n");
                m_mod_poisonpill(new_mod);
            } else {
                m_mod_log("ZzzZzz...\n");
            }
        } else if (msg->ps_msg->type == MODULE_STARTED) {
            new_mod = msg->ps_msg->sender;
            /* A new module has been started */
            const char *name = m_module_name(new_mod);
            m_mod_log("Module '%s' has been started.\n", name);
        }
    }
}
