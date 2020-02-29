#include <module/module_easy.h> 
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static void receive_sleeping(const msg_t *msg, const void *userdata);

static const self_t *new_mod;

MODULE("Doggo");

static void module_pre_start(void) {
    printf("Press 'c' to start playing with your own doggo...\n");
}

static void init(void) {
    /* Doggo should subscribe to "leaving" topic, as regex */
    m_register_src("leav[i+]ng", 0, NULL);
}

static bool check(void) {
    return true;
}

static bool evaluate(void) {
    return true;
}

static void destroy(void) {
    
}

/*
 * Default poll callback
 */
static void receive(const msg_t *msg, const void *userdata) {
    if (msg->is_pubsub) {
        switch (msg->ps_msg->type) {
        case USER:
            if (!strcmp((char *)msg->ps_msg->message, "ComeHere")) {
                m_log("Running...\n");
                m_tell_str(msg->ps_msg->sender, "BauBau");
            } else if (!strcmp((char *)msg->ps_msg->message, "LetsPlay")) {
                m_log("BauBau BauuBauuu!\n");
            } else if (!strcmp((char *)msg->ps_msg->message, "LetsEat")) {
                m_log("Burp!\n");
            } else if (!strcmp((char *)msg->ps_msg->message, "LetsSleep")) {
                m_become(sleeping);
                m_log("ZzzZzz...\n");
                
                /* Test runtime module loading */
                m_load("./libtestmod.so");
            } else if (!strcmp((char *)msg->ps_msg->message, "ByeBye")) {
                m_log("Sob...\n");
            } else if (!strcmp((char *)msg->ps_msg->message, "WakeUp")) {
                m_log("???\n");
            }
            break;
        case MODULE_STOPPED: {
                const char *name = module_get_name(msg->ps_msg->sender);
                m_log("Module '%s' has been stopped.\n", name);
            }
            break;
        default:
            break;
        }
    }
}

static void receive_sleeping(const msg_t *msg, const void *userdata) {
    if (msg->is_pubsub) {
        if (msg->ps_msg->type == USER) {
            if (!strcmp((char *)msg->ps_msg->message, "WakeUp")) {
                m_unbecome();
                m_log("Yawn...\n");
                m_poisonpill(new_mod);
            } else {
                m_log("ZzzZzz...\n");
            }
        } else if (msg->ps_msg->type == MODULE_STARTED) {
            new_mod = msg->ps_msg->sender;
            /* A new module has been started */
            const char *name = module_get_name(new_mod);
            m_log("Module '%s' has been started.\n", name);
        }
    }
}
