#include <module/ctx.h>
#include <module/mod.h>

extern void create_modules(void);

/*
 * This function is automatically called before initing any module.
 * Use this function to eg: parse config needed to decide
 * whether to start some module.
 * There is no need to explicitly call it.
 */
void m_on_boot(void) {
    printf("Press 'c' to start playing with your own doggo...\n");
}

int main(int argc, char *argv[]) {
    /*
     * Register the context
     */
    m_ctx_register("SharedSrc", 0, NULL);

    /*
     * Create the modules in the context
     */
    create_modules();

    /*
     * Loop on context to fetch modules' events
     */
    m_ctx_loop();
    
    /*
     * Finally, destroy the context, thus destroying its modules too.
     */
    m_ctx_deregister();
    return 0;
}
