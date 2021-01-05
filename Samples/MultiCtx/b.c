#include <module/mod_easy.h>
#include <module/ctx.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

extern m_ctx_t *get_B_ctx();

M_MOD("modInCtxB", get_B_ctx());

static bool init(void) {
    m_m_src_register(&((m_src_sgn_t) { SIGINT }), 0, NULL);
    return true;
}

static bool eval(void) {
    return true;
}

static void receive(const m_evt_t *msg) {
    if (msg->type == M_SRC_TYPE_SGN) {
        m_m_log("received signal %d. Leaving.\n", msg->sgn_msg->signo);
        m_m_ps_broadcast("Leave", M_PS_GLOBAL);
        m_ctx_quit(m_m_ctx(), 0);
    }
}

static void deinit(void) { }
