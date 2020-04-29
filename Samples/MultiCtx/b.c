#include <module/module_easy.h>
#include <module/context_easy.h>
#ifdef __linux__
    #include <sys/signalfd.h>
    #include <signal.h>
    #include <bits/sigaction.h>
#endif
#include <unistd.h>
#include <string.h>

static const char *myCtx = "SecondCtx";
static self_t *self;

static bool init(void) {
    m_mod_register_src(self, &((mod_sgn_t) { SIGINT }), 0, NULL);
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
        m_mod_log(self, "received signal %d. Leaving.\n", msg->sgn_msg->signo);
        m_mod_broadcast(self, "Leave", strlen("Leave"), PS_GLOBAL);
        m_ctx_quit(m_mod_ctx(self), 0);
    }
}

void create_module_B(ctx_t *c) {
    userhook_t hook = { init, eval, receive, destroy };
    m_mod_register("B", c, &self, &hook, 0);
}
