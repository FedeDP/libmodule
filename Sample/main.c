#include <module.h>

static void on_error(const char *msg);

int main() {
    modules_on_error(on_error);
    modules_loop();
    return 0;
}

static void on_error(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    modules_quit();
}
