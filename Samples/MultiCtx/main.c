#include <module.h>
#include <pthread.h>

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

static void *loop(void *param) {
    char *myCtx = (char *)param;

    /* Loop on our modules' events */
    modules_ctx_loop(myCtx);
    return NULL;
}
