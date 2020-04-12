#include <module/module_easy.h>
#include <module/modules_easy.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

static const self_t *doggo;

MODULE("Pippo");

static int myData = 5;

static void receive_ready(const msg_t *msg, const void *userdata);

static void module_pre_start(void) {

}

static bool init(void) {
    m_register_src(&((mod_sgn_t) { SIGINT }), 0, &myData);
    m_register_src(STDIN_FILENO, 0, NULL);
    
    /* Get Doggo module reference */
    m_ref("Doggo", &doggo);
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
    if (msg->type != TYPE_PS) {
        char c;
        
        /* Forcefully quit if we received a SIGINT */
        if (msg->type == TYPE_SGN) {
            c = 'q';
            int *data = (int *)userdata;
            if (data) {
                m_log("Received %d. Data is %d\n", msg->sgn_msg->signo, *data);
            }
        } else if (msg->type == TYPE_FD) {
            read(msg->fd_msg->fd, &c, sizeof(char));
        }
        
        switch (tolower(c)) {
            case 'c':
                m_log("Doggo, come here!\n");
                m_tell_str(doggo, "ComeHere", 0);
                break;
            case 'q':
                m_log("I have to go now!\n");
                m_publish_str("leaving", "ByeBye", 0);
                modules_quit(0);
                break;
            default:
                /* Avoid newline */
                if (c != 10) {
                    m_log("You got to call your doggo first. Press 'c'.\n");
                }
                break;
        }
    } else {
        if (msg->ps_msg->type == USER && !strcmp((char *)msg->ps_msg->data, "BauBau")) {
            m_become(ready);
            m_log("Press 'p' to play with Doggo! Or 'f' to feed your Doggo. 's' to have a nap. 'w' to wake him up. 'q' to leave him for now.\n");
        }
    }
}

/*
 * Secondary poll callback.
 * Use m_become(ready) to start using this second poll callback.
 */
static void receive_ready(const msg_t *msg, const void *userdata) {
    if (msg->type != TYPE_PS) {
        char c;
        
        /* Forcefully quit if we received a SIGINT */
        if (msg->type == TYPE_SGN) {
            c = 'q';
            m_log("Received %d.\n", msg->sgn_msg->signo);
        } else if (msg->type == TYPE_FD) {
            read(msg->fd_msg->fd, &c, sizeof(char));
        }
        
        switch (tolower(c)) {
            case 'p':
                m_log("Doggo, let's play a bit!\n");
                m_tell_str(doggo, "LetsPlay", 0);
                break;
            case 's':
                m_log("Doggo, you should sleep a bit!\n");
                m_tell_str(doggo, "LetsSleep", 0);
                break;
            case 'f':
                m_log("Doggo, you want some of these?\n");
                m_tell_str(doggo, "LetsEat", 0);
                break;
            case 'w':
                m_log("Doggo, wake up!\n");
                m_tell_str(doggo, "WakeUp", 0);
                break;
            case 'q':
                m_log("I have to go now!\n");
                m_publish_str("leaving", "ByeBye", 0);
                modules_quit(0);
                break;
            default:
                /* Avoid newline */
                if (c != 10) {
                    m_log("Unrecognized command. Beep. Please enter a new one... Totally not a bot.\n");
                }
                break;
        }
    }
}
