#include <module/mod_easy.h>
#include <module/ctx.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

static void receive_ready(const m_evt_t *msg);

extern m_ctx_t *get_A_ctx();

M_MOD("modInCtxA", get_A_ctx());

static int ctr;

static void logger(const m_mod_t *self, const char *fmt, va_list args) {
    const char *mname = m_mod_name(self);
    const char *cname = m_ctx_name(m_mod_ctx(self));
    if (mname && cname) {
        printf("%s@%s:\t*", mname, cname);
        vprintf(fmt, args);
    }
}

static bool init(void) {
    m_m_src_register(&((m_src_tmr_t) { CLOCK_MONOTONIC, 1000 }), 0, NULL);
    m_m_set_userdata(&ctr);
    m_ctx_set_logger(m_m_ctx(), logger);
    return true;
}

static bool eval(void) {
    return true;
}

static void receive(const m_evt_t *msg) {
    if (msg->type != M_SRC_TYPE_PS) {
        int *counter = (int *)m_m_get_userdata();
        m_m_log("recv!\n");
        (*counter)++;
    
        if (*counter % 3 == 0) {
            m_m_become(ready);
        }
    } else if (msg->ps_msg->type == M_PS_USER && !strcmp((const char *)msg->ps_msg->data, "Leave")) {
        m_m_log("Other context left. Leaving...\n");
        m_ctx_quit(m_m_ctx(), 0);
    }
}

static void deinit(void) { }

static void receive_ready(const m_evt_t *msg) {
    if (msg->type != M_SRC_TYPE_PS) {
        int *counter = (int *)m_m_get_userdata();
        m_m_log("recv2!\n");
        (*counter)++;
        if (*counter % 3 == 0) {
            m_m_unbecome();
        }
        if (*counter == 10) {
            m_m_log("Quitting context...\n");
            m_ctx_quit(m_m_ctx(), 0);
        }
    } else if (msg->ps_msg->type == M_PS_USER && !strcmp((const char *)msg->ps_msg->data, "Leave")) {
        m_m_log("Other context left. Leaving...\n");
        m_ctx_quit(m_m_ctx(), 0);
    }
}
