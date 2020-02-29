#include <module/module_easy.h> 
#include <unistd.h>
#include <string.h>

static void receive_sleeping(const msg_t *msg, const void *userdata);

MODULE("Doggo");

static void module_pre_start(void) {
}

static void init(void) {
    /* Doggo should subscribe to "leaving" topic */
    m_register_src("leaving", 0, NULL);
}

static bool check(void) {
    return true;
}

static bool evaluate(void) {
    return true;
}

static void destroy(void) {
    
}

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
            } else if (!strcmp((char *)msg->ps_msg->message, "ByeBye")) {
                m_log("Sob...\n");
            } else if (!strcmp((char *)msg->ps_msg->message, "WakeUp")) {
                m_log("???\n");
            }
            break;
        default:
            break;
        }
    }
}

static void receive_sleeping(const msg_t *msg, const void *userdata) {
    if (msg->is_pubsub && msg->ps_msg->type == USER) {
        if (!strcmp((char *)msg->ps_msg->message, "WakeUp")) {
            m_unbecome();
            m_log("Yawn...\n");
        } else {
            m_log("ZzzZzz...\n");
        }
    }
}
