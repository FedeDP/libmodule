#include "test_perf.h"
#include <module/module_easy.h>
#include <module/modules_easy.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define MAX_LEN 5000

static bool init(void);
static void my_recv(const msg_t *msg, const void *userdata);

static self_t *self = NULL;
static int ctr = 0;

void test_poll_perf(void **state) {
    (void) state; /* unused */
        
    userhook_t hook = (userhook_t) { init, NULL, my_recv, NULL };
    mod_ret ret = module_register("testName", MODULES_DEFAULT_CTX, &self, &hook);
    assert_true(ret == MOD_OK);
    assert_non_null(self);
    assert_true(module_is(self, IDLE));
    
    module_start(self);
    
    clock_t begin_tell = clock();
    for (int i = 0; i < MAX_LEN; i++) {
        module_tell(self, self, "Hello World", strlen("Hello World"), false);
    }
    clock_t end_tell = clock();
    double time_spent = (double)(end_tell - begin_tell);
    printf("Messages feeding took %.2lf us\n", time_spent);
    
    modules_loop();
    
    clock_t end_recv = clock();
    time_spent = (double)(end_recv - end_tell);
    printf("Messages fetching took %.2lf us\n", time_spent);
    
    module_deregister(&self);
}

static bool init(void) {
    return true;
}

static void my_recv(const msg_t *msg, const void *userdata) {    
    if (msg->is_pubsub && 
        msg->ps_msg->type == USER && 
        ++ctr == MAX_LEN) {
           
        modules_quit(ctr);
    }
}
