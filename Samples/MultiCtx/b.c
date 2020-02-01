#include <module/module_easy.h>
#include <module/modules.h>
#ifdef __linux__
    #include <sys/signalfd.h>
    #include <signal.h>
    #include <bits/sigaction.h>
#endif
#include <unistd.h>
#include <string.h>

static const char *myCtx = "SecondCtx";

MODULE_CTX("B", myCtx);

static void init(void) {
#ifdef __linux__
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    
    int fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    m_register_source(fd, FD_AUTOCLOSE, NULL);
#endif
}

static bool check(void) {
    return true;
}

static bool evaluate(void) {
    return true;
}

static void destroy(void) {
    
}

static void receive(const msg_t *msg, const void *userdata) {
#ifdef __linux__
    if (!msg->is_pubsub) {
        struct signalfd_siginfo fdsi;
        ssize_t s = read(msg->fd_msg->fd, &fdsi, sizeof(struct signalfd_siginfo));
        if (s != sizeof(struct signalfd_siginfo)) {
            m_log("an error occurred while getting signalfd data.\n");
        }
        m_log("received signal %d. Leaving.\n", fdsi.ssi_signo);
        m_broadcast_str("Leave", true);
        modules_ctx_quit(myCtx, 0);
    }
#endif
}
