#include <module.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

static int A_get_fd(void);
static int B_get_fd(void);
static int evaluate(void);
static void destroy(void);
static void A_recv(msg_t *msg, const void *userdata);
static void B_recv(msg_t *msg, const void *userdata);

static const void *selfA, *selfB;
static userhook hookA, hookB;

static int counter = 0;

/*
 * Create "A" and "B" modules in ctx_name context.
 * These modules can share some callbacks.
 */
void create_modules(const char *ctx_name) {
    hookA = (userhook) { A_get_fd, evaluate, A_recv, destroy };
    hookB = (userhook) { B_get_fd, evaluate, B_recv, destroy };
    
    module_register("A", ctx_name, &selfA, &hookA);
    module_register("B", ctx_name, &selfB, &hookB);
    
    module_set_userdata(selfA, ctx_name);
    module_set_userdata(selfB, ctx_name);
}

/*
 * Deregister our modules destrying them
 */
void destroy_modules(void) {
    module_deregister(&selfA);
    module_deregister(&selfB);
}

/*
 * Initializes A module's state;
 * returns a valid fd to be polled.
 */
static int A_get_fd(void) {
    int fd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
    
    struct itimerspec timerValue = {{0}};
    
    timerValue.it_value.tv_sec = 1;
    timerValue.it_interval.tv_sec = 1;
    timerfd_settime(fd, 0, &timerValue, NULL);
    return fd;
}

/*
 * Initializes B module's state;
 * returns a valid fd to be polled.
 */
static int B_get_fd(void) {
    int fd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
    
    struct itimerspec timerValue = {{0}};
    
    timerValue.it_value.tv_sec = 2;
    timerValue.it_interval.tv_sec = 2;
    timerfd_settime(fd, 0, &timerValue, NULL);
    return fd;
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
 * Destroyer function, called by module_deregister for each module.
 * Note that each module's FD is automatically closed for you.
 */
static void destroy(void) {
    
}

/*
 * Our A module's poll callback.
 */
static void A_recv(msg_t *msg, const void *userdata) {
    if (!msg->message) {
        uint64_t t;
        read(msg->fd, &t, sizeof(uint64_t));
        module_log(selfA, "recv!\n");
        
        counter++;
        if (counter == 15) {
            modules_ctx_quit((char *)userdata);
        }
    }
}

/*
 * Our B module's poll callback.
 */
static void B_recv(msg_t *msg, const void *userdata) {
    if (!msg->message) {
        uint64_t t;
        read(msg->fd, &t, sizeof(uint64_t));
        module_log(selfB, "recv!\n");
        
        counter++;
        if (counter == 15) {
            modules_ctx_quit((char *)userdata);
        }
    }
}
