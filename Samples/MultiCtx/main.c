#include <module.h>
#include <pthread.h>

static void on_error(const char *msg, const char *ctx_name);
static void *loop(void *param);

int main() {
    /* Our 2 contexts names */
    char *ctx1 = "FirstCtx";
    char *ctx2 = "SecondCtx";
    
    /* Create 2 threads that will loop on each context events */
    pthread_t th1, th2;
    pthread_create(&th1, NULL, loop, ctx1);
    pthread_create(&th2, NULL, loop, ctx2);
    
    /* Wait 2 threads to finish */
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    return 0;
}

/* Quit context on which error happened */
static void on_error(const char *msg, const char *ctx_name) {
    fprintf(stderr, "[%s] Error: %s\n", ctx_name, msg);
    modules_ctx_quit(ctx_name);
}

static void *loop(void *param) {
    char *myCtx = (char *)param;
    
    /* Set an error callback for this context */
    modules_ctx_on_error(myCtx, on_error);
    /* Loop on our modules' events */
    modules_ctx_loop(myCtx);
}
