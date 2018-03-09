#include <module.h>

static void on_error(const char *msg, const char *ctx_name);

/*
 * This function is automatically called before initing any module.
 * Use this function to eg: parse config needed to decide
 * whether to start some module.
 * There is no need to explicitly call it.
 */
void modules_pre_start() {
    printf("Prestart!\n");
}

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
