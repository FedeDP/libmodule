#include <module/module.h> 
#include <unistd.h>
#include <string.h>

static void receive_sleeping(const msg_t *msg, const void *userdata);

/* 
 * Declare and automagically initialize 
 * this module as soon as program starts.
 */
MODULE("Doggo");

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
static void init(void) {
    
}

/* 
 * Whether this module should be actually created:
 * true if module must be created, !true otherwise.
 * 
 * Use this function as a starting filter: 
 * you may desire that a module is not started in certain conditions.
 */
static int check(void) {
    return 1;
}

/* 
 * Should return not-0 value when module can be actually started (and thus polled).
 * Use this to check intra-modules dependencies or any other env variable.
 * 
 * Eg: you can evaluate your global state to make this module start right after
 * certain conditions are met.
 */
static int evaluate(void) {
    return 1;
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
    if (msg->is_pubsub) {
        switch (msg->msg->type) {
            case USER:
                if (!strcmp(msg->msg->message, "ComeHere")) {
                    m_log("Running...\n");
                    m_reply(msg->msg->sender, "BauBau");
                } else if (!strcmp(msg->msg->message, "LetsPlay")) {
                    m_log("BauBau BauuBauuu!\n");
                } else if (!strcmp(msg->msg->message, "LetsEat")) {
                    m_log("Burp!\n");
                } else if (!strcmp(msg->msg->message, "LetsSleep")) {
                    m_become(sleeping);
                    m_log("ZzzZzz...\n");
                } else if (!strcmp(msg->msg->message, "ByeBye")) {
                    m_log("Sob...\n");
                } else if (!strcmp(msg->msg->message, "WakeUp")) {
                    m_log("???\n");
                }
                break;
            case TOPIC_REGISTERED:
                /* Doggo should subscribe to "leaving" topic */
                m_subscribe(msg->msg->topic);
                break;
            default:
                break;
        }
    }
}

static void receive_sleeping(const msg_t *msg, const void *userdata) {
    if (msg->is_pubsub && msg->msg->type == USER) {
        if (!strcmp(msg->msg->message, "WakeUp")) {
            m_unbecome();
            m_log("Yawn...\n");
        } else {
            m_log("ZzzZzz...\n");
        }
    }
}
