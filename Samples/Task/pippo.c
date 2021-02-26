#include <module/mod_easy.h>
#include <module/ctx.h>

M_MOD("Pippo");

static int thData;
static int tmrData;

static int inc(void *udata);

static void module_pre_start(void) {

}

static bool init(void) {
    m_m_log("Starting data val: %d\n", thData);
    
    m_m_src_register(&((m_src_tmr_t) { CLOCK_MONOTONIC, 1000 }), 0, &tmrData);
    m_m_src_register(&((m_src_task_t) { 8, inc }), 0, &thData);
    return true;
}

static bool eval(void) {
    return true;
}

static void deinit(void) {
    
}

static void receive(const m_evt_t *msg) {
    switch (msg->type) {
    case M_SRC_TYPE_TMR: {
        int *data = (int *)msg->userdata;
        if (*data == 5) {
            m_m_log("Timed out.\n");
            m_ctx_quit(0);
            m_m_log("Final data val: %d\n", thData);
        } else {
            (*data)++;
            m_m_log("Clock... %d\n", *data);
        }
        break;
    }
    case M_SRC_TYPE_TASK:
        m_m_log("Task id: %u ended with retval: %d\n", msg->task_msg->tid, msg->task_msg->retval);
        break;
    default:
        break;
    }
}

static int inc(void *udata) {
    int *d = (int *)udata;
    while (*d < 3) {
        (*d)++;
        sleep(1);
    }
    return 0;
}
