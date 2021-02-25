#include <module/mod_easy.h>
#include <module/mem.h>
#include <module/ctx.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#ifdef __linux__
    #include <sys/inotify.h>
#else
    #include <sys/event.h>
#endif

static m_mod_t *doggo;

M_MOD("Pippo");

static int myData = 5;

static void receive_ready(const m_evt_t *msg);

static void m_mod_pre_start() {

}

static bool init(void) {
    m_m_src_register(&((m_src_sgn_t) { SIGINT }), 0, &myData);
    m_m_src_register(&((m_src_tmr_t) { CLOCK_MONOTONIC, 5000 }), M_SRC_ONESHOT, NULL);
    m_m_src_register(STDIN_FILENO, 0, NULL);
#ifdef __linux__
    m_m_src_register(&((m_src_path_t) { "/home/federico", IN_CREATE }), 0, &myData);
#else
    m_m_src_register(&((m_src_path_t) { "/home/federico", NOTE_WRITE }), 0, &myData);
#endif
    
    /* Get Doggo module reference */
    doggo = m_m_ref("Doggo");
    return true;
}

static bool eval(void) {
    return true;
}

static void deinit(void) {
    m_mem_unrefp((void **)&doggo);
}

static void receive(const m_evt_t *msg) {
    if (msg->type != M_SRC_TYPE_PS) {
        char c;
        
        /* Forcefully quit if we received a signal */
        if (msg->type == M_SRC_TYPE_SGN) {
            c = 'q';
            int *data = (int *)msg->userdata;
            if (data) {
                m_m_log("Data is %d. Received %d.\n", *data, msg->sgn_msg->signo);
            }
        } else if (msg->type == M_SRC_TYPE_TMR) {
            m_m_log("Timed out.\n");
            c = 'q';
        } else if (msg->type == M_SRC_TYPE_PATH) {
            m_m_log("A file was created in %s.\n", msg->pt_msg->path);
            c = 10;
        } else {
            (void)!read(msg->fd_msg->fd, &c, sizeof(char));
        }
        
        switch (tolower(c)) {
            case 'c':
                m_m_log("Doggo, come here!\n");
                m_m_ps_tell(doggo, "ComeHere", 0);
                break;
            case 'q':
                m_m_log("I have to go now!\n");
                m_m_ps_publish("leaving", "ByeBye", 0);
                m_ctx_quit(0);
                break;
            default:
                /* Avoid newline */
                if (c != 10) {
                    m_m_log("You got to call your doggo first. Press 'c'.\n");
                }
                break;
        }
    } else if (msg->ps_msg->type == M_PS_USER && 
        !strcmp((char *)msg->ps_msg->data, "BauBau")) {
        
        m_ctx_dump();
        
        m_m_become(ready);
        m_m_log("Press 'p' to play with Doggo! Or 'f' to feed your Doggo. 's' to have a nap. 'w' to wake him up. 'q' to leave him for now.\n");
    }
}

/*
 * Secondary poll callback.
 * Use m_become(ready) to start using this second poll callback.
 */
static void receive_ready(const m_evt_t *msg) {
    if (msg->type != M_SRC_TYPE_PS) {
        char c = 10;
        
        /* Forcefully quit if we received a signal */
        if (msg->type == M_SRC_TYPE_SGN) {
            c = 'q';
            m_m_log("Received %d. Quit.\n", msg->sgn_msg->signo);
        } else if (msg->type == M_SRC_TYPE_FD) {
            (void)!read(msg->fd_msg->fd, &c, sizeof(char));
        } else if (msg->type == M_SRC_TYPE_TMR) {
            m_m_log("Timer expired.\n");
        } else if (msg->type == M_SRC_TYPE_PATH) {
            m_m_log("A file was created in %s.\n", msg->pt_msg->path);
        }
        
        switch (tolower(c)) {
            case 'p':
                m_m_log("Doggo, let's play a bit!\n");
                m_m_ps_tell(doggo, "LetsPlay", 0);
                break;
            case 's':
                m_m_log("Doggo, you should sleep a bit!\n");
                m_m_ps_tell(doggo, "LetsSleep", 0);
                break;
            case 'f':
                m_m_log("Doggo, you want some of these?\n");
                m_m_ps_tell(doggo, "LetsEat", 0);
                break;
            case 'w':
                m_m_log("Doggo, wake up!\n");
                m_m_ps_tell(doggo, "WakeUp", 0);
                break;
            case 'q':
                m_m_dump();
                m_m_log("I have to go now!\n");
                m_m_ps_publish("leaving", "ByeBye", 0);
                m_ctx_quit(0);
                break;
            default:
                /* Avoid newline */
                if (c != 10) {
                    m_m_log("Unrecognized command. Beep. Please enter a new one... Totally not a bot.\n");
                }
                break;
        }
    }
}
