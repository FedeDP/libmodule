#include <module.h>
#include <modules.h>
// #include <module/module.h>
// #include <module/modules.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/* 
 * Note how check() function is not required now 
 * as we're explicitly calling module_register 
 */
static void A_init(void);
static void B_init(void);
static int evaluate(void);
static void destroy(void);
static void A_recv(const msg_t *msg, const void *userdata);
static void A_recv_ready(const msg_t *msg, const void *userdata);
static void B_recv(const msg_t *msg, const void *userdata);
static void B_recv_sleeping(const msg_t *msg, const void *userdata);

static const self_t *selfA, *selfB;

/*
 * Create "A" and "B" modules in ctx_name context.
 * These modules can share some callbacks.
 */
void create_modules(const char *ctx_name) {
    userhook hookA = (userhook) { A_init, evaluate, A_recv, destroy };
    userhook hookB = (userhook) { B_init, evaluate, B_recv, destroy };
    
    module_register("Pippo", ctx_name, &selfA, &hookA);
    module_register("Doggo", ctx_name, &selfB, &hookB);
    
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

/*
 * Initializes A module's state;
 * returns a valid fd to be polled.
 */
static void A_init(void) {
    module_add_fd(selfA, STDIN_FILENO);
}

/*
 * Initializes B module's state;
 * returns a valid fd to be polled.
 */
static void B_init(void) {
    /* Doggo is subscribed to "leaving" topic */
    module_subscribe(selfB, "leaving");
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
 * Destroyer function, called by module_deregister for each module.
 * Note that each module's FD is automatically closed for you.
 */
static void destroy(void) {
    
}

/*
 * Our A module's poll callback.
 */
static void A_recv(const msg_t *msg, const void *userdata) {
    if (!msg->is_pubsub) {
        char c;
        read(msg->fd, &c, sizeof(char));
        
        switch (tolower(c)) {
            case 'c':
                module_log(selfA, "Doggo, come here!\n");
                module_tell(selfA, "Doggo", "ComeHere");
                break;
            default:
                /* Avoid newline */
                if (c != 10) {
                    module_log(selfA, "You got to call your doggo first. Press 'c'.\n");
                }
                break;
        }
    } else {
        if (!strcmp(msg->msg->message, "BauBau")) {
            module_become(selfA, A_recv_ready);
            module_log(selfA, "Press 'p' to play with Doggo! Or 'f' to feed your Doggo. 's' to have a nap. 'w' to wake him up. 'q' to leave him for now.\n");
        }
    }
}

static void A_recv_ready(const msg_t *msg, const void *userdata) {
    if (!msg->is_pubsub) {
        char c;
        read(msg->fd, &c, sizeof(char));
        
        switch (tolower(c)) {
            case 'p':
                module_log(selfA, "Doggo, let's play a bit!\n");
                module_tell(selfA, "Doggo", "LetsPlay");
                break;
            case 's':
                module_log(selfA, "Doggo, you should sleep a bit!\n");
                module_tell(selfA, "Doggo", "LetsSleep");
                break;
            case 'f':
                module_log(selfA, "Doggo, you want some of these?\n");
                module_tell(selfA, "Doggo", "LetsEat");
                break;
            case 'w':
                module_log(selfA, "Doggo, wake up!\n");
                module_tell(selfA, "Doggo", "WakeUp");
                break;
            case 'q':
                module_log(selfA, "I have to go now!\n");
                module_publish(selfA, "leaving", "ByeBye");
                modules_ctx_quit("test");
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
    if (msg->is_pubsub) {
        if (!strcmp(msg->msg->message, "ComeHere")) {
            module_log(selfB, "Running...\n");
            module_reply(selfB, msg->msg->sender, "BauBau");
        } else if (!strcmp(msg->msg->message, "LetsPlay")) {
            module_log(selfB, "BauBau BauuBauuu!\n");
        } else if (!strcmp(msg->msg->message, "LetsEat")) {
            module_log(selfB, "Burp!\n");
        } else if (!strcmp(msg->msg->message, "LetsSleep")) {
            module_become(selfB, B_recv_sleeping);
            module_log(selfB, "ZzzZzz...\n");
        } else if (!strcmp(msg->msg->message, "ByeBye")) {
            module_log(selfB, "Sob...\n");
        } else if (!strcmp(msg->msg->message, "WakeUp")) {
            module_log(selfB, "???\n");
        }
    }
}

static void B_recv_sleeping(const msg_t *msg, const void *userdata) {
    if (msg->is_pubsub) {
        if (!strcmp(msg->msg->message, "WakeUp")) {
            module_become(selfB, B_recv);
            module_log(selfB, "Yawn...\n");
        } else {
            module_log(selfB, "ZzzZzz...\n");
        }
    }
}
