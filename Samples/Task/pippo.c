#include <module/module_easy.h>
#include <module/context_easy.h>
#include <assert.h>

M_MOD("Pippo");

static int thData;
static int tmrData;

static int inc(void *udata);

static void module_pre_start(void) {

}

static bool init(void) {
    m_m_log("Starting data val: %d\n", thData);
    
    m_m_register_src(&((mod_tmr_t) { CLOCK_MONOTONIC, 1000 }), 0, &tmrData);
    m_m_register_src(&((mod_task_t) { 8, inc }), 0, &thData);
    return true;
}

static bool check(void) {
    return true;
}

static bool eval(void) {
    return true;
}

static void destroy(void) {
    
}

static void receive(const msg_t *msg, const void *userdata) {
    switch (msg->type) {
    case TYPE_TMR: {
        int *data = (int *)userdata;
        if (*data == 5) {
            m_m_log("Timed out.\n");
            m_c_quit(0);
            m_m_log("Final data val: %d\n", thData);
        } else {
            (*data)++;
            m_m_log("Clock... %d\n", *data);
        }
        break;
    }
    case TYPE_TASK:
        m_m_log("Task id: %u ended with retval: %d\n", msg->task_msg->tid, msg->task_msg->retval);
        break;
    default:
        break;
    }
}

static int inc(void *udata) {
    /* 
     * YOU CANNOT CALL libmodule API
     * from a different thread where it was registered.
     */
    assert(m_mod_name() == NULL);
    int *d = (int *)udata;
    while (*d < 3) {
        (*d)++;
        sleep(1);
    }
    return 0;
}
