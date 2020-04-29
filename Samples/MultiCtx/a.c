#include <module/module_easy.h>
#include <module/context_easy.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

static const char *myCtx = "FirstCtx";

static self_t *self;

static void m_mod_pre_start(void) {
    
}

static void receive_ready(const msg_t *msg, const void *userdata);

static bool init(void) {
    m_mod_register_src(self, &((mod_tmr_t) { CLOCK_MONOTONIC, 1000 }), 0, NULL);
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

static void receive(const msg_t *msg, const void *userdata) {
    if (msg->type != TYPE_PS) {
        static int counter = 0;
        m_mod_log(self, "recv!\n");
        counter++;
    
        if (counter % 3 == 0) {
            m_mod_become(self, receive_ready);
            m_mod_set_userdata(self, &counter);
        }
    } else if (msg->ps_msg->type == USER && !strcmp((const char *)msg->ps_msg->data, "Leave")) {
        m_mod_log(self, "Other context left. Leaving...\n");
        m_ctx_quit(m_mod_ctx(self), 0);
    }
}

static void receive_ready(const msg_t *msg, const void *userdata) {
    if (msg->type != TYPE_PS) {
        int *counter = (int *)m_mod_get_userdata(self);
        m_mod_log(self, "recv2!\n");
        (*counter)++;
        if (*counter % 3 == 0) {
            m_mod_unbecome(self);
        }
        if (*counter == 10) {
            m_mod_log(self, "Quitting...\n");
            m_ctx_quit(m_mod_ctx(self), 0);
        }
    } else if (msg->ps_msg->type == USER && !strcmp((const char *)msg->ps_msg->data, "Leave")) {
        m_mod_log(self, "Other context left. Leaving...\n");
        m_ctx_quit(m_mod_ctx(self), 0);
    }
}

void create_module_A(ctx_t *c) {
    userhook_t hook = { init, eval, receive, destroy };
    m_mod_register("A", c, &self, &hook, 0);
}
