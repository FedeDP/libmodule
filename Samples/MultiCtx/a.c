#include <module/mod_easy.h>
#include <module/ctx.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

static const char *myCtx = "FirstCtx";
static m_mod_t *mod;

static void m_mod_pre_start(void) {
    
}

static void receive_ready(const m_evt_t *msg);

static bool init(void) {
    m_mod_src_register(mod, &((m_src_tmr_t) { CLOCK_MONOTONIC, 1000 }), 0, NULL);
    return true;
}

static bool eval(void) {
    return true;
}

static void destroy(void) {
    
}

static void receive(const m_evt_t *msg) {
    if (msg->type != M_SRC_TYPE_PS) {
        int *counter = (int *)m_mod_get_userdata(mod);
        m_mod_log(mod, "recv!\n");
        (*counter)++;
    
        if (*counter % 3 == 0) {
            m_mod_become(mod, receive_ready);
        }
    } else if (msg->ps_msg->type == M_PS_USER && !strcmp((const char *)msg->ps_msg->data, "Leave")) {
        m_mod_log(mod, "Other context left. Leaving...\n");
        m_ctx_quit(m_mod_ctx(mod), 0);
    }
}

static void receive_ready(const m_evt_t *msg) {
    if (msg->type != M_SRC_TYPE_PS) {
        int *counter = (int *)m_mod_get_userdata(mod);
        m_mod_log(mod, "recv2!\n");
        (*counter)++;
        if (*counter % 3 == 0) {
            m_mod_unbecome(mod);
        }
        if (*counter == 10) {
            m_mod_log(mod, "Quitting...\n");
            m_ctx_quit(m_mod_ctx(mod), 0);
        }
    } else if (msg->ps_msg->type == M_PS_USER && !strcmp((const char *)msg->ps_msg->data, "Leave")) {
        m_mod_log(mod, "Other context left. Leaving...\n");
        m_ctx_quit(m_mod_ctx(mod), 0);
    }
}

void create_module_A(m_ctx_t *c) {
    static int counter;
    m_userhook_t hook = { init, eval, receive, destroy };
    m_mod_register("A", c, &mod, &hook, 0, &counter);
}
