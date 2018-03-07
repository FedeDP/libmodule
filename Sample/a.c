#include <module.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdint.h>

MODULE(A);

static void recv_ready(message_t *msg, const void *userdata);

static int init(void) {
    int fd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
    
    struct itimerspec timerValue = {{0}};
    
    timerValue.it_value.tv_sec = 1;
    timerValue.it_interval.tv_sec = 1;
    timerfd_settime(fd, 0, &timerValue, NULL);
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
