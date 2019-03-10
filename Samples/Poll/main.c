#include <module/modules_easy.h>
#include <poll.h>

/*
 * This function is automatically called before initing any module.
 * Use this function to eg: parse config needed to decide
 * whether to start some module.
 * There is no need to explicitly call it.
 */
void modules_pre_start() {
    printf("Started Libmodule %d.%d.%d\n", MODULE_VERSION_MAJ, MODULE_VERSION_MIN, MODULE_VERSION_PAT);
    printf("Press 'c' to start playing with your own doggo...\n");
}

int main() {
    int ret = 0;
    int fd = 0;
    
    /* Get default context fd */
    if (modules_get_fd(&fd) != MOD_OK) {
        /* No context! */
        return 1;
    }
    
    /* Initial dispatch */
    if (modules_dispatch(&ret) != MOD_OK) {
        return 1;
    }
    
    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN;
    
    while (ret >= 0) {
        ret = poll(&fds, 1, -1);
        if (ret > 0) {
            if (fds.revents & POLLIN) {
                if (modules_dispatch(&ret) != MOD_OK) {
                    if (ret >= 0) { // modules_quit(int retval) -> "retval" is returned here
                        printf("Loop: return code %d.\n", ret);
                    } else { // An error happened
                        printf("Loop: error happened.\n");
                    }
                    ret = -1; // leave
                } else {
                    printf("Loop: dispatched %d messages.\n", ret);
                }
            }
        }
    }
    close(fd);
    return 0;
}
