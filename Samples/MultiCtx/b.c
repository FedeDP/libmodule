#include <module/module_easy.h>
#include <module/modules.h>
#ifdef __linux__
    #include <sys/signalfd.h>
    #include <signal.h>
#endif
#include <unistd.h>

static const char *myCtx = "SecondCtx";
/* 
 * Declare and automagically initialize 
 * this module and its context as soon as program starts.
 */
MODULE_CTX("B", myCtx);

/*
 * This function is automatically called before registering the module. 
 * Use this to set some  global state needed eg: in check() function 
 */
static void module_pre_start(void) {
    
}

/*
 * Initializes this module's state;
 * returns a valid fd to be polled.
 */
static void init(void) {
#ifdef __linux__
    sigset_t mask;
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    int fd = signalfd(-1, &mask, 0);
    m_register_fd(fd, 1, NULL);
#endif
}

/* 
 * Whether this module should be actually created:
 * true if module must be created, !true otherwise.
 * 
 * Use this function as a starting filter: 
 * you may desire that a module is not started in certain conditions.
 */
static bool check(void) {
    return true;
}

/* 
 * Should return not-0 value when module can be actually started (and thus polled).
 * Use this to check intra-modules dependencies or any other env variable.
 * 
 * Eg: you can evaluate your global state to make this module start right after
 * certain conditions are met.
 */
static bool evaluate(void) {
    return true;
}

/*
 * Destroyer function, called at module unload (at end of program).
 * Note that module's FD is automatically closed for you.
 */
static void destroy(void) {
    
}

/*
 * Default poll callback
 */
static void receive(const msg_t *msg, const void *userdata) {
#ifdef __linux__
    if (!msg->is_pubsub) {
        struct signalfd_siginfo fdsi;
        ssize_t s = read(msg->fd_msg->fd, &fdsi, sizeof(struct signalfd_siginfo));
        if (s != sizeof(struct signalfd_siginfo)) {
            m_log("an error occurred while getting signalfd data.\n");
        }
        m_log("received signal %d. Leaving.\n", fdsi.ssi_signo);
        modules_ctx_quit(myCtx, 0);
    }
#endif
}
