#include <module.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdint.h>

/* 
 * Declare and automagically initialize 
 * this module as soon as program starts.
 * Note that module's name is not passed as string here.
 */
MODULE(A);

static void recv_ready(message_t *msg, const void *userdata);

/*
 * Initializes this module's state;
 * returns a valid fd to be polled.
 */
static int init(void) {
    int fd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
    
    struct itimerspec timerValue = {{0}};
    
    timerValue.it_value.tv_sec = 1;
    timerValue.it_interval.tv_sec = 1;
    timerfd_settime(fd, 0, &timerValue, NULL);
    return fd;
}

/* 
 * Whether this module should be actually created:
 * 0 means OK -> start this module
 * !0 means do not start.
 * 
 * Use this function as a starting filter: 
 * you may desire that a module is not started in certain conditions.
 */
static int check(void) {
    return 0;
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
 * Our default poll callback.
 * Note that message_t->msg/sender are unused for now.
 */
static void recv(message_t *msg, const void *userdata) {
    if (!msg->message) {
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
static void recv_ready(message_t *msg, const void *userdata) {
    if (!msg->message) {
        uint64_t t;
        read(msg->fd, &t, sizeof(uint64_t));
    
        int *counter = (int *)userdata;
        m_log("recv2!\n");
        (*counter)++;
    
        if (*counter % 3 == 0) {
            m_unbecome();
        }
    }
}
