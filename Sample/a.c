#include <module.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdint.h>

MODULE(A);

int init(void) {
    int fd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
    
    struct itimerspec timerValue = {{0}};
    
    timerValue.it_value.tv_sec = 1;
    timerValue.it_interval.tv_sec = 1;
    timerfd_settime(fd, 0, &timerValue, NULL);
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
    uint64_t t;
    read(fd, &t, sizeof(uint64_t));
    module_log("callback!\n");
}
