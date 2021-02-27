#include "test_perf.h"
#include <module/mod.h>
#include <module/ctx.h>
#include <string.h>
#include <time.h>

#define MAX_LEN 5000

static bool init(void);
static void my_recv(const m_evt_t *msg);

static m_mod_t *mod;
static int ctr;
static bool warned;
static m_mod_stats_t stats;

void test_poll_perf(void **state) {
    (void) state; /* unused */
        
    m_mod_hook_t hook = {init, NULL, my_recv, NULL };
    int ret = m_mod_register("testName", &mod, &hook, 0, NULL);
    assert_true(ret == 0);
    assert_non_null(mod);
    assert_true(m_mod_is(mod, M_MOD_IDLE));
    
    m_mod_start(mod);

    m_mod_set_thresh(mod, &(m_mod_thresh_t){ .activity_freq = 10.00 });
    
    clock_t begin_tell = clock();
    for (int i = 0; i < MAX_LEN; i++) {
        m_mod_ps_tell(mod, mod, "Hello World", 0);
    }
    clock_t end_tell = clock();
    double time_spent = (double)(end_tell - begin_tell);
    printf("Messages feeding took %.2lf us\n", time_spent);
    
    m_ctx_loop();
    
    clock_t end_recv = clock();
    time_spent = (double)(end_recv - end_tell);
    printf("Messages fetching took %.2lf us\n", time_spent);

    assert_true(warned);
    m_mod_thresh_t thresh = {0};
    m_mod_get_thresh(mod, &thresh);
    printf("Was warned as activity was beyond thresh: %.2lf > %.2lf.\n", stats.activity_freq, thresh.activity_freq);

    m_mod_deregister(&mod);
}

static bool init(void) {
    return true;
}

static void my_recv(const m_evt_t *msg) {    
    if (msg->type == M_SRC_TYPE_PS) {
        if (msg->ps_msg->type == M_PS_USER && ++ctr == MAX_LEN) {
            m_ctx_quit(0);
        } else if (msg->ps_msg->type == M_PS_MOD_THRESH_ACTIVITY) {
            warned = true;
            m_mod_stats(mod, &stats);
        }
    }
}
