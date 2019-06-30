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

/* 
 * Declare and automagically initialize 
 * this module as soon as program starts.
 */
MODULE("Pippo");

static int myData = 5;

static void receive_ready(const msg_t *msg, const void *userdata);

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
 * Note that module's FD is automatically closed for you.
 */
static void destroy(void) {
    
}

/*
 * Default poll callback
 */
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
                m_tell_str(doggo, "ComeHere", false);
                break;
            case 'q':
                m_log("I have to go now!\n");
                m_publish_str("leaving", "ByeBye", false);
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
        if (msg->pubsub_msg->type == USER && !strcmp((char *)msg->pubsub_msg->message, "BauBau")) {
            m_become(ready);
            /* Finally register Leaving topic */
            m_register_topic("leaving");
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
                m_tell_str(doggo, "LetsPlay", false);
                break;
            case 's':
                m_log("Doggo, you should sleep a bit!\n");
                m_tell_str(doggo, "LetsSleep", false);
                break;
            case 'f':
                m_log("Doggo, you want some of these?\n");
                m_tell_str(doggo, "LetsEat", false);
                break;
            case 'w':
                m_log("Doggo, wake up!\n");
                m_tell_str(doggo, "WakeUp", false);
                break;
            case 'q':
                m_dump();
                m_log("I have to go now!\n");
                m_publish_str("leaving", "ByeBye", false);
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
