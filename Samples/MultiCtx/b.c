#include <module/mod_easy.h>
#include <module/ctx.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static const char *myCtx = "SecondCtx";
static m_mod_t *mod;

static bool init(void) {
    m_mod_src_register(mod, &((mod_sgn_t) { SIGINT }), 0, NULL);
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

static void receive(const m_evt_t *msg, const void *userdata) {
    if (msg->type != M_SRC_TYPE_PS) {
        m_mod_log(mod, "received signal %d. Leaving.\n", msg->sgn_msg->signo);
        m_mod_ps_broadcast(mod, "Leave", M_PS_GLOBAL);
        m_ctx_quit(m_mod_ctx(mod), 0);
    }
}

void create_module_B(m_ctx_t *c) {
    m_userhook_t hook = { init, eval, receive, destroy };
    m_mod_register("B", c, &mod, &hook, 0, NULL);
}
