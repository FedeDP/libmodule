#include <module/module.h>
#include <module/modules.h>
#ifdef __linux__
    #include <sys/timerfd.h>
#endif
#include <unistd.h>
#include <stdint.h>

static const char *myCtx = "FirstCtx";
/* 
 * Declare and automagically initialize 
 * this module and its context as soon as program starts.
 */
MODULE_CTX("A", myCtx);

/*
 * This function is automatically called before registering the module. 
 * Use this to set some  global state needed eg: in check() function 
 */
static void module_pre_start(void) {
    
}

static void receive_ready(const msg_t *msg, const void *userdata);

/*
 * Initializes this module's state;
 * returns a valid fd to be polled.
 */
static void init(void) {
#ifdef __linux__
    int fd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
    
    struct itimerspec timerValue = {{0}};
    
    timerValue.it_value.tv_sec = 1;
    timerValue.it_interval.tv_sec = 1;
    timerfd_settime(fd, 0, &timerValue, NULL);
    m_register_fd(fd, 1);
#endif
}

/* 
 * Whether this module should be actually created:
 * true if module must be created, !true otherwise.
 * 
 * Use this function as a starting filter: 
 * you may desire that a module is not started in certain conditions.
 */
static int check(void) {
    return 1;
}

/* 
 * Should return not-0 value when module can be actually started (and thus polled).
 * Use this to check intra-modules dependencies or any other env variable.
 * 
 * Eg: you can evaluate your global state to make this module start right after
 * certain conditions are met.
 */
static int evaluate(void) {
    return 1;
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
    if (!msg->is_pubsub) {
        uint64_t t;
        read(msg->fd, &t, sizeof(uint64_t));
    
        static int counter = 0;
        m_log("recv!\n");
        counter++;
    
        if (counter % 3 == 0) {
            m_become(ready);
            m_set_userdata(&counter);
        }
    }
}

/*
 * Secondary poll callback.
 * Use m_become(ready) to start using this second poll callback.
 */
static void receive_ready(const msg_t *msg, const void *userdata) {
    if (!msg->is_pubsub) {
        uint64_t t;
        read(msg->fd, &t, sizeof(uint64_t));
    
        int *counter = (int *)userdata;
        m_log("recv2!\n");
        (*counter)++;
    
        if (*counter % 3 == 0) {
            m_unbecome();
        }
        if (*counter == 10) {
            modules_ctx_quit(myCtx, 0);
        }
    }
}
