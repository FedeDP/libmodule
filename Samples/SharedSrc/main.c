#include <module/modules.h>

extern void create_modules(const char *ctx_name);
extern void destroy_modules(void);

/*
 * This function is automatically called before initing any module.
 * Use this function to eg: parse config needed to decide
 * whether to start some module.
 * There is no need to explicitly call it.
 */
void modules_pre_start() {
    printf("Press 'c' to start playing with your own doggo...\n");
}

int main() {
    /* 
     * Firstly, create our desired modules.
     * We will use "test" as new context name.
     * New context will be created as soon as first module is added to it.
     */
    create_modules("test");

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
