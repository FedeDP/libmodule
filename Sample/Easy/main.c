#include <module.h>

static void on_error(const char *msg, const char *ctx_name);

int main() {
    modules_on_error(on_error);
    modules_loop();
    return 0;
}

static void on_error(const char *msg, const char *ctx_name) {
    fprintf(stderr, "[%s] Error: %s\n", ctx_name, msg);
    modules_quit();
}
