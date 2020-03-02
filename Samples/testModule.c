#include <module/module_easy.h>
#include <string.h>

/** This is a test module needed for Easy example, to runtime link another module **/

MODULE("Test");

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
    m_register_src("leaving", 0, NULL);
    m_log("Linked.\n");
    return true;
}

/* 
 * Whether this module should be actually created:
 * true if module must be created, !true otherwise.
 * 
 * Use this function as a starting filter: 
 * you may desire that a module is not started in certain conditions.
 */
static bool check(void) {
    return true;
}

/* 
 * Should return not-0 value when module can be actually started (and thus polled).
 * Use this to check intra-modules dependencies or any other env variable.
 * 
 * Eg: you can evaluate your global state to make this module start right after
 * certain conditions are met.
 */
static bool evaluate(void) {
    return true;
}

/*
 * Destroyer function, called at module unload (at end of program).
 * Note that any module's fds are automatically closed for you.
 */
static void destroy(void) {

}

/*
 * Default poll callback
 */
static void receive(const msg_t *msg, const void *userdata) {
    if (msg->is_pubsub && msg->ps_msg->type == USER) {
        if (!strcmp((char *)msg->ps_msg->message, "ByeBye")) {
            m_log("Received quit.\n");
        }
    }
}
