#include "test_evt_ref.h"
#include <module/mod.h>
#include <module/ctx.h>
#include <module/mem/mem.h>

static void my_recv(m_mod_t *mod, const m_queue_t *const evts);

static m_mod_t *mod = NULL;
static m_evt_t *ref;

void test_evt_ref(void **state) {
    (void) state; /* unused */

    test_ctx = NULL;
    int ret = m_ctx_register("evt_ref", &test_ctx, 0, NULL);
    assert_true(ret == 0);
    assert_non_null(test_ctx);

    m_mod_hook_t hook = { .on_evt = my_recv };
    ret = m_mod_register("testName", test_ctx, &mod, &hook, 0, NULL);
    assert_true(ret == 0);
    assert_non_null(mod);
    assert_true(m_mod_is(mod, M_MOD_IDLE));
    
    m_mod_start(mod);
    m_mod_ps_tell(mod, mod, "Hello World", 0);
    
    m_ctx_loop(test_ctx);
    
    /* Test that ref event is not nil */
    assert_non_null(ref);
    ref = m_mem_unref(ref);
    assert_null(ref);
    
    ret = m_mod_deregister(&mod);
    assert_int_equal(ret, 0);
}

static void my_recv(m_mod_t *mod, const m_queue_t *const evts) {
    m_itr_foreach(evts, {
        m_evt_t *msg = m_itr_get(m_itr);
        if (msg->type == M_SRC_TYPE_PS) {

            ref = m_mem_ref((void *)msg);
            m_ctx_quit(test_ctx,0);
        }
    });
}

