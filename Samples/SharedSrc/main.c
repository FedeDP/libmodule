#include <module/ctx.h>

extern void create_modules(m_ctx_t *c);
extern void destroy_modules(void);

/*
 * This function is automatically called before initing any module.
 * Use this function to eg: parse config needed to decide
 * whether to start some module.
 * There is no need to explicitly call it.
 */
void m_pre_start() {
    printf("Press 'c' to start playing with your own doggo...\n");
}

int main(int argc, char *argv[]) {
    /*
     * Register the context
     */
    m_ctx_t *c = NULL;
    m_ctx_register("SharedSrc", &c, 0, NULL);

    /*
     * Create the modules in the context
     */
    create_modules(c);

    /*
     * Loop on context to fetch modules' events
     */
    m_ctx_loop(c);
    
    /*
     * Finally, destroy our modules.
     * As context had no M_CTX_PERSIST flag,
     * it will be automatically destroyed when it has no module in it.
     */
    destroy_modules();
    return 0;
}
