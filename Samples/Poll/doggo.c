#include <module/mod_easy.h> 
#include <string.h>

static void m_m_on_evt_sleeping(const m_evt_t *msg);

M_M("Doggo");

static void m_m_pre_start(void) {

}

static bool m_m_on_start(void) {
    /* Doggo should subscribe to "leaving" topic */
    m_m_src_register("leaving", 0, NULL);
    return true;
}

static bool m_m_on_eval(void) {
    return true;
}

static void m_m_on_stop(void) {
    
}

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
            } else if (!strcmp((char *)msg->ps_evt->data, "ByeBye")) {
                m_m_log("Sob...\n");
            } else if (!strcmp((char *)msg->ps_evt->data, "WakeUp")) {
                m_m_log("???\n");
            }
            break;
        default:
            break;
        }
    }
}

static void m_m_on_evt_sleeping(const m_evt_t *msg) {
    if (msg->type == M_SRC_TYPE_PS && msg->ps_evt->type == M_PS_USER) {
        if (!strcmp((char *)msg->ps_evt->data, "WakeUp")) {
            m_m_unbecome();
            m_m_log("Yawn...\n");
        } else {
            m_m_log("ZzzZzz...\n");
        }
    }
}
