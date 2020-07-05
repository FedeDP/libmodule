#include <module/mod_easy.h> 
#include <unistd.h>
#include <string.h>

static void receive_sleeping(const msg_t *msg, const void *userdata);

M_MOD("Doggo");

static void module_pre_start(void) {
}

static bool init(void) {
    /* Doggo should subscribe to "leaving" topic */
    m_m_register_src("leaving", 0, NULL);
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

static void receive(const msg_t *msg, const void *userdata) {
    if (msg->type == M_SRC_TYPE_PS) {
        switch (msg->ps_msg->type) {
        case USER:
            if (!strcmp((char *)msg->ps_msg->data, "ComeHere")) {
                m_m_log("Running...\n");
                m_m_tell(msg->ps_msg->sender, "BauBau", 0);
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsPlay")) {
                m_m_log("BauBau BauuBauuu!\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsEat")) {
                m_m_log("Burp!\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "LetsSleep")) {
                m_m_become(sleeping);
                m_m_log("ZzzZzz...\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "ByeBye")) {
                m_m_log("Sob...\n");
            } else if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
                m_m_log("???\n");
            }
            break;
        default:
            break;
        }
    }
}

static void receive_sleeping(const msg_t *msg, const void *userdata) {
    if (msg->type == M_SRC_TYPE_PS && msg->ps_msg->type == USER) {
        if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
            m_m_unbecome();
            m_m_log("Yawn...\n");
        } else {
            m_m_log("ZzzZzz...\n");
        }
    }
}
