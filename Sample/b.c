#include <module.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>

MODULE(B);

int init(void) {
    sigset_t mask;
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    int fd = signalfd(-1, &mask, 0);
    // FIXME: this macro will crash here
//     module_log("starting on fd: %d.\n", fd);
    return fd;
}

int check(void) {
    return 0;
}

int state_change(void) {
    return 1;
}

void destroy(void) {
    
}

void callback(int fd) {
    struct signalfd_siginfo fdsi;    
    ssize_t s = read(fd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s != sizeof(struct signalfd_siginfo)) {
        module_log("an error occurred while getting signalfd data.\n");
    }
    module_log("received signal %d. Leaving.\n", fdsi.ssi_signo);
    modules_quit();
}
