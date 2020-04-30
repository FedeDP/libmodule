#include <module/mod_easy.h>
#include <module/ctx.h>
#ifdef __linux__
    #include <sys/signalfd.h>
    #include <signal.h>
    #include <bits/sigaction.h>
#endif
#include <unistd.h>
#include <string.h>

static const char *myCtx = "SecondCtx";
static mod_t *mod;

static bool init(void) {
    m_mod_register_src(mod, &((mod_sgn_t) { SIGINT }), 0, NULL);
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
        m_mod_log(mod, "received signal %d. Leaving.\n", msg->sgn_msg->signo);
        m_mod_broadcast(mod, "Leave", PS_GLOBAL);
        m_ctx_quit(m_mod_ctx(mod), 0);
    }
}

void create_module_B(ctx_t *c) {
    userhook_t hook = { init, eval, receive, destroy };
    m_mod_register("B", c, &mod, &hook, 0);
}
