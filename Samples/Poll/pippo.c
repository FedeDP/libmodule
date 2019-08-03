#include <module/module_easy.h>
#include <module/modules_easy.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#ifdef __linux__
    #include <sys/signalfd.h>
    #include <signal.h>
#endif

static const self_t *doggo;

MODULE("Pippo");

static int myData = 5;

static void receive_ready(const msg_t *msg, const void *userdata);

static void module_pre_start(void) {

}

static void init(void) {
#ifdef __linux__
    /* Add signal fd */
    sigset_t mask;
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    int fd = signalfd(-1, &mask, 0);
    m_register_fd(fd, 1, &myData);
#endif
    
    /* Add stdin fd */
    m_register_fd(STDIN_FILENO, 0, NULL);
    
    /* Get Doggo module reference */
    m_ref("Doggo", &doggo);
}

static bool check(void) {
    return true;
}

static bool evaluate(void) {
    return true;
}

static void destroy(void) {
    
}

static void receive(const msg_t *msg, const void *userdata) {
    if (!msg->is_pubsub) {
        char c;
        
        /* Forcefully quit if we received a signal */
        if (msg->fd_msg->fd != STDIN_FILENO) {
            c = 'q';
            int *data = (int *)msg->fd_msg->userptr;
            if (data) {
                m_log("Data is %d\n", *data);
            }
        } else {
            read(msg->fd_msg->fd, &c, sizeof(char));
        }
        
        switch (tolower(c)) {
            case 'c':
                m_log("Doggo, come here!\n");
                m_tell_str(doggo, "ComeHere");
                break;
            case 'q':
                m_log("I have to go now!\n");
                m_publish_str("leaving", "ByeBye");
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
        if (msg->ps_msg->type == USER && !strcmp((char *)msg->ps_msg->message, "BauBau")) {
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
    if (!msg->is_pubsub) {
        char c;
        
        /* Forcefully quit if we received a signal */
        if (msg->fd_msg->fd != STDIN_FILENO) {
            c = 'q';
        } else {
            read(msg->fd_msg->fd, &c, sizeof(char));
        }
        
        switch (tolower(c)) {
            case 'p':
                m_log("Doggo, let's play a bit!\n");
                m_tell_str(doggo, "LetsPlay");
                break;
            case 's':
                m_log("Doggo, you should sleep a bit!\n");
                m_tell_str(doggo, "LetsSleep");
                break;
            case 'f':
                m_log("Doggo, you want some of these?\n");
                m_tell_str(doggo, "LetsEat");
                break;
            case 'w':
                m_log("Doggo, wake up!\n");
                m_tell_str(doggo, "WakeUp");
                break;
            case 'q':
                m_log("I have to go now!\n");
                m_publish_str("leaving", "ByeBye");
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
