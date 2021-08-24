#include <module/ctx.h>
#include <poll.h>

/*
 * This function is automatically called before registering any module.
 * Use this function to eg: parse config needed to decide
 * whether to start some module.
 * There is no need to explicitly call it.
 */
void m_pre_start() {
    printf("Poll_Sample -> Started Libmodule %d.%d.%d\n", LIBMODULE_VERSION_MAJ, LIBMODULE_VERSION_MIN, LIBMODULE_VERSION_PAT);
    printf("Press 'c' to start playing with your own doggo...\n");
}

int main(int argc, char *argv[]) {
    int ret = 0;
    
    /* Initial dispatch */
    if (m_ctx_dispatch(NULL) != 0) {
        return 1;
    }

    /* Get context fd */
    int fd = m_ctx_fd(NULL);
    if (fd < 0) {
        return 1;
    }

    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN;
    
    while (ret >= 0) {
        ret = poll(&fds, 1, -1);
        if (ret > 0) {
            if (fds.revents & POLLIN) {
                ret = m_ctx_dispatch(NULL);
                if (ret < 0) {
                    printf("Loop: error happened.\n");
                } else if (ret > 0) {
                    printf("Loop: dispatched %d messages.\n", ret);
                } else {
                    printf("Loop exited.\n");
                    break;
                }
            }
        }
    }
    close(fd);
    return 0;
}
