#include <module/module_easy.h>
#include <module/modules.h>
#ifdef __linux__
    #include <sys/timerfd.h>
#endif
#include <unistd.h>
#include <stdint.h>
#include <string.h>

static const char *myCtx = "FirstCtx";

MODULE_CTX("A", myCtx);

static void module_pre_start(void) {
    
}

static void receive_ready(const msg_t *msg, const void *userdata);

static void init(void) {
#ifdef __linux__
    int fd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
    
    struct itimerspec timerValue = {{0}};
    
    timerValue.it_value.tv_sec = 1;
    timerValue.it_interval.tv_sec = 1;
    timerfd_settime(fd, 0, &timerValue, NULL);
    m_register_fd(fd, true, NULL);
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
    if (!msg->is_pubsub) {
        uint64_t t;
        read(msg->fd_msg->fd, &t, sizeof(uint64_t));
    
        static int counter = 0;
        m_log("recv!\n");
        counter++;
    
        if (counter % 3 == 0) {
            m_become(ready);
            m_set_userdata(&counter);
        }
    } else if (msg->ps_msg->type == USER && !strcmp((const char *)msg->ps_msg->message, "Leave")) {
        m_log("Other context left. Leaving...\n");
        modules_ctx_quit(myCtx, 0);
    }
}

static void receive_ready(const msg_t *msg, const void *userdata) {
    if (!msg->is_pubsub) {
        uint64_t t;
        read(msg->fd_msg->fd, &t, sizeof(uint64_t));
    
        int *counter = (int *)userdata;
        m_log("recv2!\n");
        (*counter)++;
    
        if (*counter % 3 == 0) {
            m_unbecome();
        }
        if (*counter == 10) {
            modules_ctx_quit(myCtx, 0);
        }
    } else if (msg->ps_msg->type == USER && !strcmp((const char *)msg->ps_msg->message, "Leave")) {
        m_log("Other context left. Leaving...\n");
        modules_ctx_quit(myCtx, 0);
    }
}
