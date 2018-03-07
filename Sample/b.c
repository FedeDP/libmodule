#include <module.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>

MODULE(B);

static int init(void) {
    sigset_t mask;
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    int fd = signalfd(-1, &mask, 0);
    m_log("starting on fd: %d.\n", fd);
    return fd;
}

static int check(void) {
    return 0;
}

static int evaluate(void) {
    return 1;
}

static void destroy(void) {
    
}

static void recv(message_t *msg, const void *userdata) {
    if (!msg->message) {
        struct signalfd_siginfo fdsi;    
        ssize_t s = read(msg->fd, &fdsi, sizeof(struct signalfd_siginfo));
        if (s != sizeof(struct signalfd_siginfo)) {
            m_log("an error occurred while getting signalfd data.\n");
        }
        m_log("received signal %d. Leaving.\n", fdsi.ssi_signo);
        modules_quit();
    }
}
