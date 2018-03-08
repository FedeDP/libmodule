#include <module.h>

extern void create_modules(const char *ctx_name);
extern void destroy_modules(void);

static void on_error(const char *msg, const char *ctx_name);

int main() {
    /* 
     * Firstly, create our desired modules.
     * We will use "test" as new context name.
     * New context will be created as soon as first module is added to it.
     */
    create_modules("test");
    
    /*
     * Intercept any error in "test" context with our error callback.
     * Note that context must be available at this stage.
     */
    modules_ctx_on_error("test", on_error);
    
    /*
     * Loop on this context to get our modules' events
     */
    modules_ctx_loop("test");
    
    /*
     * Finally, destroy our modules
     */
    destroy_modules();
    return 0;
}

static void on_error(const char *msg, const char *ctx_name) {
    fprintf(stderr, "[%s] Error: %s\n", ctx_name, msg);
    modules_ctx_quit("test");
}
