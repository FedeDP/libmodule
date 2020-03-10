#include <module/module_easy.h> 
#include <unistd.h>
#include <string.h>

static void receive_sleeping(const msg_t *msg, const void *userdata);

MODULE("Doggo");

static void module_pre_start(void) {
}

static bool init(void) {
    /* Doggo should subscribe to "leaving" topic */
    m_register_src("leaving", 0, NULL);
    return true;
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
    if (msg->type == TYPE_PS) {
        switch (msg->ps_msg->type) {
        case USER:
            if (!strcmp((char *)msg->ps_msg->data, "ComeHere")) {
                m_log("Running...\n");
                m_tell_str(msg->ps_msg->sender, "BauBau", 0);
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsPlay")) {
                m_log("BauBau BauuBauuu!\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsEat")) {
                m_log("Burp!\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsSleep")) {
                m_become(sleeping);
                m_log("ZzzZzz...\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "ByeBye")) {
                m_log("Sob...\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
                m_log("???\n");
            }
            break;
        default:
            break;
        }
    }
}

static void receive_sleeping(const msg_t *msg, const void *userdata) {
    if (msg->type == TYPE_PS && msg->ps_msg->type == USER) {
        if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
            m_unbecome();
            m_log("Yawn...\n");
        } else {
            m_log("ZzzZzz...\n");
        }
    }
}
