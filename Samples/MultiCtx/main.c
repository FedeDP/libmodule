#include <module/context_easy.h>
#include <module/module.h>
#include <pthread.h>
#include <stdlib.h>
#ifdef __linux__
    #include <signal.h>
    #include <bits/sigaction.h>
#endif

static void *loop(void *param);

static void logger(const self_t *self, const char *fmt, va_list args) {
    const char *mname = module_name(self);
    const char *cname = module_ctx(self);
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
    
    /* Set a different logger for ctx1 context */
    m_context_set_logger(ctx1, logger);
    
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

    /* Loop on our modules' events */
    m_context_loop(myCtx);
    return NULL;
}
