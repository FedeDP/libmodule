#include <modules.h>
#include <module.h>
// #include <module/modules.h>
#include <pthread.h>
#include <stdlib.h>

static void *loop(void *param);

static void logger(const self_t *self, const char *fmt, va_list args, const void *userdata) {
    char *mname = NULL, *cname = NULL;
    if (module_get_name(self, &mname) == MOD_OK &&
        module_get_context(self, &cname) == MOD_OK) {
       
        printf("%s@%s:\t*", mname, cname);
        vprintf(fmt, args);
        free(mname);
        free(cname);
    }
}

int main() {
    /* Our 2 contexts names */
    char *ctx1 = "FirstCtx";
    char *ctx2 = "SecondCtx";
    
    /* Set a different logger for ctx1 context */
    modules_ctx_set_logger(ctx1, logger);
    
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
