#include <module/modules_easy.h>
#include <poll.h>

/*
 * This function is automatically called before initing any module.
 * Use this function to eg: parse config needed to decide
 * whether to start some module.
 * There is no need to explicitly call it.
 */
void modules_pre_start() {
    printf("Poll_Sample -> Started Libmodule %d.%d.%d\n", MODULE_VERSION_MAJ, MODULE_VERSION_MIN, MODULE_VERSION_PAT);
    printf("Press 'c' to start playing with your own doggo...\n");
}

int main(int argc, char *argv[]) {
    int ret = 0;
    
    /* Initial dispatch */
    if (modules_dispatch() != 0) {
        return 1;
    }
    
    /* Get default context fd */
    int fd = modules_fd();
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
                ret = modules_dispatch();
                if (ret < 0) {
                    printf("Loop: error happened.\n");
                } else if (ret > 0) {
                    printf("Loop: dispatched %d messages.\n", ret);
                } else {
                    printf("Loop exited.\n");
                }
            }
        }
    }
    close(fd);
    return 0;
}
