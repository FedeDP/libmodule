#include <module.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdint.h>

MODULE(A);

static void callback2(int fd);

static int init(void) {
    int fd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
    
    struct itimerspec timerValue = {{0}};
    
    timerValue.it_value.tv_sec = 1;
    timerValue.it_interval.tv_sec = 1;
    timerfd_settime(fd, 0, &timerValue, NULL);
    // FIXME: this macro will crash here
//     module_log("starting on fd: %d.\n", fd);
    return fd;
}

static int check(void) {
    return 0;
}

static int state_change(void) {
    return 1;
}

static void destroy(void) {
    
}

void callback(int fd) {
    uint64_t t;
    read(fd, &t, sizeof(uint64_t));
    
    static int counter = 0;
    module_log("callback!\n");
    counter++;
    
    if (counter % 5 == 0) {
        userhook *hook = m_get_hook();
        hook->pollCb = callback2;
    }
}

static void callback2(int fd) {
    uint64_t t;
    read(fd, &t, sizeof(uint64_t));
    
    static int counter = 0;
    module_log("callback2!\n");
    counter++;
    
    if (counter % 3 == 0) {
        userhook *hook = m_get_hook();
        hook->pollCb = callback;
    }
}
