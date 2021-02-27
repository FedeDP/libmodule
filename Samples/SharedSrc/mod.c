#include <module/mod.h>
#include <module/ctx.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/* 
 * Note how check() function is not required now 
 * as we're explicitly calling module_register 
 */
static bool A_init(void);
static bool B_init(void);
static bool evaluate(void);
static void destroy(void);
static void A_recv(const m_evt_t *msg);
static void A_recv_ready(const m_evt_t *msg);
static void B_recv(const m_evt_t *msg);
static void B_recv_sleeping(const m_evt_t *msg);

static m_mod_t *selfA, *selfB;

/*
 * Create "A" and "B" modules in ctx_name context.
 * These modules can share some callbacks.
 */
void create_modules(void) {
    m_mod_hook_t hookA = (m_mod_hook_t) {A_init, evaluate, A_recv, destroy };
    m_mod_hook_t hookB = (m_mod_hook_t) {B_init, evaluate, B_recv, destroy };
    
    m_mod_register("Pippo", &selfA, &hookA, 0, NULL);
    m_mod_register("Doggo", &selfB, &hookB, 0, NULL);
}

/*
 * Deregister our modules destrying them
 */
void destroy_modules(void) {
    m_mod_deregister(&selfA);
    m_mod_deregister(&selfB);
}

static bool A_init(void) {
    m_mod_src_register(selfA, STDIN_FILENO, 0, NULL);
    return true;
}

static bool B_init(void) {
    /* Doggo should subscribe to "leaving" topic */
    m_mod_src_register(selfB, "leaving", 0, NULL);
    return true;
}

static bool evaluate(void) {
    return true;
}

static void destroy(void) {
    
}

/*
 * Our A module's poll callback.
 */
static void A_recv(const m_evt_t *msg) {
    if (msg->type != M_SRC_TYPE_PS) {
        char c;
        (void)!read(msg->fd_msg->fd, &c, sizeof(char));
        
        switch (tolower(c)) {
            case 'c':
                m_mod_log(selfA, "Doggo, come here!\n");
                m_mod_ps_tell(selfA, selfB, (unsigned char *)"ComeHere", 0);
                break;
            case 'q':
                m_mod_log(selfA, "I have to go now!\n");
                m_mod_ps_publish(selfA, "leaving", (unsigned char *)"ByeBye", 0);
                m_ctx_quit(0);
                break;
            default:
                /* Avoid newline */
                if (c != 10) {
                    m_mod_log(selfA, "You got to call your doggo first. Press 'c'.\n");
                }
                break;
        }
    } else {
        if (msg->ps_msg->type == M_PS_USER && !strcmp((char *)msg->ps_msg->data, "BauBau")) {
            m_mod_become(selfA, A_recv_ready);
            m_mod_log(selfA, "Press 'p' to play with Doggo! Or 'f' to feed your Doggo. 's' to have a nap. 'w' to wake him up. 'q' to leave him for now.\n");
        }
    }
}

static void A_recv_ready(const m_evt_t *msg) {
    if (msg->type != M_SRC_TYPE_PS) {
        char c;
        (void)!read(msg->fd_msg->fd, &c, sizeof(char));
        
        switch (tolower(c)) {
            case 'p':
                m_mod_log(selfA, "Doggo, let's play a bit!\n");
                m_mod_ps_tell(selfA, selfB, (unsigned char *)"LetsPlay", 0);
                break;
            case 's':
                m_mod_log(selfA, "Doggo, you should sleep a bit!\n");
                m_mod_ps_tell(selfA, selfB, (unsigned char *)"LetsSleep", 0);
                break;
            case 'f':
                m_mod_log(selfA, "Doggo, you want some of these?\n");
                m_mod_ps_tell(selfA, selfB, (unsigned char *)"LetsEat", 0);
                break;
            case 'w':
                m_mod_log(selfA, "Doggo, wake up!\n");
                m_mod_ps_tell(selfA, selfB, (unsigned char *)"WakeUp", 0);
                break;
            case 'q':
                m_mod_log(selfA, "I have to go now!\n");
                m_mod_ps_publish(selfA, "leaving", (unsigned char *)"ByeBye", 0);
                m_ctx_quit(0);
                break;
            default:
                /* Avoid newline */
                if (c != 10) {
                    m_mod_log(selfA, "Unrecognized command. Beep. Please enter a new one... Totally not a bot.\n");
                }
                break;
        }
    }
}

/*
 * Our B module's poll callback.
 */
static void B_recv(const m_evt_t *msg) {
    if (msg->type == M_SRC_TYPE_PS) {
        switch (msg->ps_msg->type) {
            case M_PS_USER:
                if (!strcmp((char *)msg->ps_msg->data, "ComeHere")) {
                    m_mod_log(selfB, "Running...\n");
                    m_mod_ps_tell(selfB, msg->ps_msg->sender, (unsigned char *)"BauBau", 0);
                } else if (!strcmp((char *)msg->ps_msg->data, "LetsPlay")) {
                    m_mod_log(selfB, "BauBau BauuBauuu!\n");
                } else if (!strcmp((char *)msg->ps_msg->data, "LetsEat")) {
                    m_mod_log(selfB, "Burp!\n");
                } else if (!strcmp((char *)msg->ps_msg->data, "LetsSleep")) {
                    m_mod_become(selfB, B_recv_sleeping);
                    m_mod_log(selfB, "ZzzZzz...\n");
                } else if (!strcmp((char *)msg->ps_msg->data, "ByeBye")) {
                    m_mod_log(selfB, "Sob...\n");
                } else if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
                    m_mod_log(selfB, "???\n");
                }
                break;
            default:
                break;
        }
    }
}

static void B_recv_sleeping(const m_evt_t *msg) {
    if (msg->type == M_SRC_TYPE_PS && msg->ps_msg->type == M_PS_USER) {
        if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
            m_mod_become(selfB, B_recv);
            m_mod_log(selfB, "Yawn...\n");
        } else {
            m_mod_log(selfB, "ZzzZzz...\n");
        }
    }
}
