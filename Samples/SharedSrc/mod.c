#include <module/module.h>
#include <module/context.h>
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
static void A_recv(const msg_t *msg, const void *userdata);
static void A_recv_ready(const msg_t *msg, const void *userdata);
static void B_recv(const msg_t *msg, const void *userdata);
static void B_recv_sleeping(const msg_t *msg, const void *userdata);

static self_t *selfA, *selfB;

/*
 * Create "A" and "B" modules in ctx_name context.
 * These modules can share some callbacks.
 */
void create_modules(const char *ctx_name) {
    userhook_t hookA = (userhook_t) { A_init, evaluate, A_recv, destroy };
    userhook_t hookB = (userhook_t) { B_init, evaluate, B_recv, destroy };
    
    module_register("Pippo", ctx_name, &selfA, &hookA, 0);
    module_register("Doggo", ctx_name, &selfB, &hookB, 0);
    
    module_set_userdata(selfA, ctx_name);
    module_set_userdata(selfB, ctx_name);
}

/*
 * Deregister our modules destrying them
 */
void destroy_modules(void) {
    module_deregister(&selfA);
    module_deregister(&selfB);
}

static bool A_init(void) {
    module_register_src(selfA, STDIN_FILENO, 0, NULL);
    return true;
}

static bool B_init(void) {
    /* Doggo should subscribe to "leaving" topic */
    module_register_src(selfB, "leaving", 0, NULL);
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
static void A_recv(const msg_t *msg, const void *userdata) {
    if (msg->type != TYPE_PS) {
        char c;
        read(msg->fd_msg->fd, &c, sizeof(char));
        
        switch (tolower(c)) {
            case 'c':
                module_log(selfA, "Doggo, come here!\n");
                module_tell(selfA, selfB, (unsigned char *)"ComeHere", strlen("ComeHere"), false);
                break;
            case 'q':
                module_log(selfA, "I have to go now!\n");
                module_publish(selfA, "leaving", (unsigned char *)"ByeBye", strlen("ByeBye"), false);
                m_context_quit("test", 0);
                break;
            default:
                /* Avoid newline */
                if (c != 10) {
                    module_log(selfA, "You got to call your doggo first. Press 'c'.\n");
                }
                break;
        }
    } else {
        if (msg->ps_msg->type == USER  && !strcmp((char *)msg->ps_msg->data, "BauBau")) {
            module_become(selfA, A_recv_ready);
            module_log(selfA, "Press 'p' to play with Doggo! Or 'f' to feed your Doggo. 's' to have a nap. 'w' to wake him up. 'q' to leave him for now.\n");
        }
    }
}

static void A_recv_ready(const msg_t *msg, const void *userdata) {
    if (msg->type != TYPE_PS) {
        char c;
        read(msg->fd_msg->fd, &c, sizeof(char));
        
        switch (tolower(c)) {
            case 'p':
                module_log(selfA, "Doggo, let's play a bit!\n");
                module_tell(selfA, selfB, (unsigned char *)"LetsPlay", strlen("LetsPlay"), false);
                break;
            case 's':
                module_log(selfA, "Doggo, you should sleep a bit!\n");
                module_tell(selfA, selfB, (unsigned char *)"LetsSleep", strlen("LetsSleep"), false);
                break;
            case 'f':
                module_log(selfA, "Doggo, you want some of these?\n");
                module_tell(selfA, selfB, (unsigned char *)"LetsEat", strlen("LetsEat"), false);
                break;
            case 'w':
                module_log(selfA, "Doggo, wake up!\n");
                module_tell(selfA, selfB, (unsigned char *)"WakeUp", strlen("WakeUp"), false);
                break;
            case 'q':
                module_log(selfA, "I have to go now!\n");
                module_publish(selfA, "leaving", (unsigned char *)"ByeBye", strlen("ByeBye"), false);
                m_context_quit("test", 0);
                break;
            default:
                /* Avoid newline */
                if (c != 10) {
                    module_log(selfA, "Unrecognized command. Beep. Please enter a new one... Totally not a bot.\n");
                }
                break;
        }
    }
}

/*
 * Our B module's poll callback.
 */
static void B_recv(const msg_t *msg, const void *userdata) {
    if (msg->type == TYPE_PS) {
        switch (msg->ps_msg->type) {
            case USER:
                if (!strcmp((char *)msg->ps_msg->data, "ComeHere")) {
                    module_log(selfB, "Running...\n");
                    module_tell(selfB, msg->ps_msg->sender, (unsigned char *)"BauBau", strlen("BauBau"), false);
                } else if (!strcmp((char *)msg->ps_msg->data, "LetsPlay")) {
                    module_log(selfB, "BauBau BauuBauuu!\n");
                } else if (!strcmp((char *)msg->ps_msg->data, "LetsEat")) {
                    module_log(selfB, "Burp!\n");
                } else if (!strcmp((char *)msg->ps_msg->data, "LetsSleep")) {
                    module_become(selfB, B_recv_sleeping);
                    module_log(selfB, "ZzzZzz...\n");
                } else if (!strcmp((char *)msg->ps_msg->data, "ByeBye")) {
                    module_log(selfB, "Sob...\n");
                } else if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
                    module_log(selfB, "???\n");
                }
                break;
            default:
                break;
        }
    }
}

static void B_recv_sleeping(const msg_t *msg, const void *userdata) {
    if (msg->type == TYPE_PS && msg->ps_msg->type == USER) {
        if (!strcmp((char *)msg->ps_msg->data, "WakeUp")) {
            module_become(selfB, B_recv);
            module_log(selfB, "Yawn...\n");
        } else {
            module_log(selfB, "ZzzZzz...\n");
        }
    }
}
