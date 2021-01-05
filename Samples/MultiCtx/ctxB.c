#include <module/ctx_easy.h>
#ifdef __linux__
    #include <signal.h>
    #include <bits/sigaction.h>
#endif

M_CTX("B");

m_ctx_t *get_B_ctx() {
    return m_ctx;
}

void m_pre_start(void) {
#ifdef __linux__
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
#endif
}
