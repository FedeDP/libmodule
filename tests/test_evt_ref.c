#include "test_evt_ref.h"
#include <module/mod.h>
#include <module/ctx.h>
#include <module/mem.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

static bool init(void);
static void my_recv(const m_evt_t *const msg);

static m_mod_t *mod = NULL;
static m_evt_t *ref;

void test_evt_ref(void **state) {
    (void) state; /* unused */
    
    test_ctx = NULL;
    
    int ret = m_ctx_register(M_CTX_DEFAULT, &test_ctx, 0, NULL);
    assert_true(ret == 0);
    assert_non_null(test_ctx);
        
    m_userhook_t hook = { init, NULL, my_recv, NULL };
    ret = m_mod_register("testName", test_ctx, &mod, &hook, 0, NULL);
    assert_true(ret == 0);
    assert_non_null(mod);
    assert_true(m_mod_is(mod, M_MOD_IDLE));
    
    m_mod_start(mod);
    m_mod_ps_tell(mod, mod,  "Hello World", 0);
    
    m_ctx_loop(test_ctx, M_CTX_MAX_EVENTS);
    
    /* Test that ref event is not nil */
    assert_non_null(ref);
    ref = m_mem_unref(ref);
    assert_null(ref);
    
    m_mod_deregister(&mod);
}

static bool init(void) {
    return true;
}

static void my_recv(const m_evt_t *const msg) {  
    if (msg->type == M_SRC_TYPE_PS && 
        msg->ps_msg->type == M_PS_USER) {

        ref = m_mem_ref((void *)msg);
        m_ctx_quit(test_ctx, 0);
    }
}

