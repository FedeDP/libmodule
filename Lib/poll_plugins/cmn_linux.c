#include "poll_priv.h"
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <linux/version.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>

/* Inotify related defines */
#define BUF_LEN (sizeof(struct inotify_event) + NAME_MAX + 1)

void create_timerfd(ev_src_t *tmp) {
    const int abs_fl = tmp->flags & SRC_TMR_ABSOLUTE ? TFD_TIMER_ABSTIME : 0;
    tmp->tm_src.f.fd = timerfd_create(tmp->tm_src.its.clock_id, TFD_NONBLOCK | TFD_CLOEXEC | abs_fl);
    
    struct itimerspec timerValue = {{0}};
    timerValue.it_value.tv_sec = tmp->tm_src.its.ms / 1000;
    timerValue.it_value.tv_nsec = (tmp->tm_src.its.ms % 1000) * 1000 * 1000;
    if (!(tmp->flags & SRC_ONESHOT)) {
        /* Set interval */
        timerValue.it_interval.tv_sec = tmp->tm_src.its.ms / 1000;
        timerValue.it_interval.tv_nsec = (tmp->tm_src.its.ms % 1000) * 1000 * 1000;
    }
    timerfd_settime(tmp->tm_src.f.fd, 0, &timerValue, NULL);
}

void create_signalfd(ev_src_t *tmp) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, tmp->sgn_src.sgs.signo);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    tmp->sgn_src.f.fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
}

void create_inotifyfd(ev_src_t *tmp) {
    tmp->pt_src.f.fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    inotify_add_watch(tmp->pt_src.f.fd, tmp->pt_src.pt.path, tmp->pt_src.pt.events);
}

void create_pidfd(ev_src_t *tmp) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
    tmp->pt_src.f.fd = syscall(__NR_pidfd_open, tmp->pid_src.pid.pid, 0);
#endif
}

void reset_fd(ev_src_t *tmp) {
    int *fd = NULL;
    switch (tmp->type) {
    case TYPE_TMR:
        fd = &tmp->tm_src.f.fd;
        break;
    case TYPE_SGN:
        fd = &tmp->sgn_src.f.fd;
        break;
    case TYPE_PT:
        fd = &tmp->pt_src.f.fd;
        break;
    case TYPE_PID:
        fd = &tmp->pid_src.f.fd;
        break;
    default:
        break;
    }
    if (fd) {
        close(*fd);
        *fd = -1;
    }
}

mod_ret poll_consume_sgn(poll_priv_t *priv, const int idx, ev_src_t *src, sgn_msg_t *sgn_msg) {
    struct signalfd_siginfo fdsi;
    ssize_t s = read(src->sgn_src.f.fd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s == sizeof(struct signalfd_siginfo)) {
        return MOD_OK;
    }
    return MOD_ERR;
}

mod_ret poll_consume_tmr(poll_priv_t *priv, const int idx, ev_src_t *src, tm_msg_t *tm_msg) {
    uint64_t t;
    if (read(src->tm_src.f.fd, &t, sizeof(uint64_t)) == sizeof(uint64_t)) {
        return MOD_OK;
    }
    return MOD_ERR;
}

mod_ret poll_consume_pt(poll_priv_t *priv, const int idx, ev_src_t *src, pt_msg_t *pt_msg) {
    char buffer[BUF_LEN];
    const size_t length = read(src->pt_src.f.fd, buffer, BUF_LEN);
    if (length > 0) {
        struct inotify_event *event = (struct inotify_event *) buffer;
        if (event->len) {
            pt_msg->events = event->mask;
            return MOD_OK;
        }
    }
    return MOD_ERR;
}

mod_ret poll_consume_pid(poll_priv_t *priv, const int idx, ev_src_t *src, pid_msg_t *pid_msg) {
    pid_msg->events = 0;
    return MOD_OK; // nothing to do
}
