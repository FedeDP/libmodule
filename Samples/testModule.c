#include <module/mod_easy.h>
#include <string.h>

/** This is a test module needed for Easy example, to runtime link another module **/

M_MOD("Test", NULL);

/*
 * This function is automatically called before registering the module. 
 * Use this to set some  global state needed eg: in check() function 
 */
static void module_pre_start(void) {
}

/*
 * Initializes this module's state;
 * returns a valid fd to be polled.
 */
static bool init(void) {
    m_m_src_register("leaving", 0, NULL);
    m_m_log("Linked.\n");
    return true;
}

/* 
 * Should return not-0 value when module can be actually started (and thus polled).
 * Use this to check intra-modules dependencies or any other env variable.
 * 
 * Eg: you can evaluate your global state to make this module start right after
 * certain conditions are met.
 */
static bool eval(void) {
    return true;
}

/*
 * Destroyer function, called at module unload (at end of program).
 * Note that any module's fds are automatically closed for you.
 */
static void deinit(void) {

}

/*
 * Default poll callback
 */
static void receive(const m_evt_t *msg) {
    if (msg->type == M_SRC_TYPE_PS && msg->ps_msg->type == M_PS_USER) {
        if (!strcmp((char *)msg->ps_msg->data, "ByeBye")) {
            m_m_log("Received quit.\n");
        }
    }
}
