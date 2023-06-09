#define _POSIX_C_SOURCE 200809L

#include <module/mod_easy.h>
#include <unistd.h>
#include <time.h>

M_MOD("Pippo");

static int thData;
static int tmrData;

static int inc(void *udata);

/* Hook on m_ctx_pre_loop() weak symbol */
void m_ctx_pre_loop(int argc, char *argv[]) {
    printf("Task example starting.\n");
}

/* Hook on m_ctx_post_loop() weak symbol */
void m_ctx_post_loop(int argc, char *argv[]) {
    printf("Task example ended.\n");
}

static void m_mod_on_boot(void) {
    printf("Booting task example.\n");
}

static bool m_mod_on_start(m_mod_t *mod) {
    m_mod_log(mod,"Starting data val: %d\n", thData);
    
    m_mod_src_register(mod, &((m_src_tmr_t) { CLOCK_MONOTONIC, (uint64_t)1 * 1000 * 1000 * 1000 }), 0, &tmrData); // 1s
    m_mod_src_register(mod, &((m_src_task_t) { 8, inc }), 0, &thData);
    m_mod_batch_set(mod, 0, 1500); // 1500ms!
    return true;
}

static bool m_mod_on_eval(m_mod_t *mod) {
    return true;
}

static void m_mod_on_stop(m_mod_t *mod) {
    
}

static void m_mod_on_evt(m_mod_t *mod, const m_queue_t *const evts) {
    m_itr_foreach(evts, {
        m_evt_t *msg = m_itr_get(m_itr);
        switch (msg->type) {
        case M_SRC_TYPE_TMR: {
            int *data = (int *)msg->userdata;
            if (*data == 5) {
                m_mod_log(mod, "Timed out.\n");
                m_ctx_quit(0);
                m_mod_log(mod,"Final data val: %d\n", thData);
            } else {
                (*data)++;
                m_mod_log(mod, "Clock... %d\n", *data);
            }
            break;
        }
        case M_SRC_TYPE_TASK:
            m_mod_log(mod,"Task id: %u ended with retval: %d\n", msg->task_evt->tid, msg->task_evt->retval);
            break;
        default:
            break;
        }
    });
}

static int inc(void *udata) {
    int *d = (int *)udata;
    while (*d < 3) {
        (*d)++;
        sleep(1);
    }
    return 0;
}
