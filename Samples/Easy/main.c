#include <module.h>

static void on_error(const char *msg, const char *ctx_name);

int main() {
    /* Set an error callback */
    modules_on_error(on_error);
    /* Loop on our modules' events */
    modules_loop();
    return 0;
}

static void on_error(const char *msg, const char *ctx_name) {
    fprintf(stderr, "[%s] Error: %s\n", ctx_name, msg);
    modules_quit();
}
