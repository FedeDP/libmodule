#include <module/ctx.h>
#include <module/mod.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
    #include <signal.h>
    #include <bits/sigaction.h>
#endif

extern void create_module_A(m_ctx_t *c);
extern void create_module_B(m_ctx_t *c);

static void *loop(void *param);

static void logger(const m_mod_t *self, const char *fmt, va_list args) {
    const char *mname = m_mod_name(self);
    const char *cname = m_ctx_name(m_mod_ctx(self));
    if (mname && cname) {
        printf("%s@%s:\t*", mname, cname);
        vprintf(fmt, args);
    }
}

void setup_signals(void) {
#ifdef __linux__
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
#endif
}

int main(int argc, char *argv[]) {
    /* Properly block signals (that are received by mod "B" on ctx2) */
    setup_signals();
        
    /* Our 2 contexts names */
    char *ctx1 = "FirstCtx";
    char *ctx2 = "SecondCtx";
    
    /* Create 2 threads that will loop on each context events */
    pthread_t th1, th2;
    pthread_create(&th1, NULL, loop, ctx1);
    pthread_create(&th2, NULL, loop, ctx2);
    
    /* Wait 2 threads to finish */
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    return 0;
}

static void *loop(void *param) {
    char *myCtx = (char *)param;
    
    m_ctx_t *c = NULL;
    m_ctx_register(myCtx, &c, M_CTX_NAME_DUP, NULL);
    if (!strcmp(myCtx, "FirstCtx")) {
        /* Set a different logger for ctx1 context */
        m_ctx_set_logger(c, logger);
        create_module_A(c);
    } else {
        create_module_B(c);
    }

    /* Loop on our modules' events */
    m_ctx_loop(c, M_CTX_MAX_EVENTS);
    return NULL;
}
