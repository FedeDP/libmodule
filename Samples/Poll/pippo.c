#include <module/mod_easy.h>
#include <module/mem/mem.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

static m_mod_t *doggo;

M_MOD("Pippo");

static int myData = 5;

static void m_mod_on_evt_ready(m_mod_t *mod, const m_queue_t *const evts);

static void m_mod_on_boot(void) {

}

static bool m_mod_on_start(m_mod_t *mod) {
    m_mod_src_register(mod, &((m_src_sgn_t) { SIGINT }), 0, &myData);
    m_mod_src_register(mod, STDIN_FILENO, 0, NULL);
    
    /* Get Doggo reference */
    doggo = m_mod_lookup(mod, "Doggo");
    return true;
}

static bool m_mod_on_eval(m_mod_t *mod) {
    return true;
}

static void m_mod_on_stop(m_mod_t *mod) {

}

static void m_mod_on_evt(m_mod_t *mod, const m_queue_t *const evts) {
    m_itr_foreach(evts, {
        m_evt_t *msg = m_itr_get(m_itr);
        if (msg->type != M_SRC_TYPE_PS) {
            char c = 'q';
            
            /* Forcefully quit if we received a SIGINT */
            if (msg->type == M_SRC_TYPE_SGN) {
                c = 'q';
                int *data = (int *)msg->userdata;
                if (data) {
                    m_mod_log(mod, "Received %d. Data is %d\n", msg->sgn_evt->signo, *data);
                }
            } else if (msg->type == M_SRC_TYPE_FD) {
                (void)!read(msg->fd_evt->fd, &c, sizeof(char));
            }
            
            switch (tolower(c)) {
                case 'c':
                    m_mod_log(mod,"Doggo, come here!\n");
                    m_mod_ps_tell(mod, doggo, "ComeHere", 0);
                    break;
                case 'q':
                    m_mod_log(mod,"I have to go now!\n");
                    m_mod_ps_publish(mod, "leaving", "ByeBye", 0);
                    m_ctx_quit(0);
                    break;
                default:
                    /* Avoid newline */
                    if (c != 10) {
                        m_mod_log(mod,"You got to call your doggo first. Press 'c'.\n");
                    }
                    break;
            }
        } else {
            if (strcmp((char *)msg->ps_evt->data, "BauBau") == 0) {
                m_mod_become(mod, m_mod_on_evt_ready);
                m_mod_log(mod,"Press 'p' to play with Doggo! Or 'f' to feed your Doggo. 's' to have a nap. 'w' to wake him up. 'q' to leave him for now.\n");
            }
        }
    });
}

/*
 * Secondary poll callback.
 * Use m_become(ready) to start using this second poll callback.
 */
static void m_mod_on_evt_ready(m_mod_t *mod, const m_queue_t *const evts) {
    m_itr_foreach(evts, {
        m_evt_t *msg = m_itr_get(m_itr);
        if (msg->type != M_SRC_TYPE_PS) {
            char c = 'q';
            
            /* Forcefully quit if we received a SIGINT */
            if (msg->type == M_SRC_TYPE_SGN) {
                c = 'q';
                m_mod_log(mod,"Received %d.\n", msg->sgn_evt->signo);
            } else if (msg->type == M_SRC_TYPE_FD) {
                (void)!read(msg->fd_evt->fd, &c, sizeof(char));
            }
            
            switch (tolower(c)) {
                case 'p':
                    m_mod_log(mod,"Doggo, let's play a bit!\n");
                    m_mod_ps_tell(mod, doggo, "LetsPlay", 0);
                    m_ctx_dump();
                    break;
                case 's':
                    m_mod_log(mod,"Doggo, you should sleep a bit!\n");
                    m_mod_ps_tell(mod, doggo, "LetsSleep", 0);
                    break;
                case 'f':
                    m_mod_log(mod, "Doggo, you want some of these?\n");
                    m_mod_ps_tell(mod, doggo, "LetsEat", 0);
                    break;
                case 'w':
                    m_mod_log(mod, "Doggo, wake up!\n");
                    m_mod_ps_tell(mod, doggo, "WakeUp", 0);
                    break;
                case 'q':
                    m_mod_log(mod, "I have to go now!\n");
                    m_mod_ps_publish(mod, "leaving", "ByeBye", 0);
                    m_ctx_quit(0);
                    break;
                default:
                    /* Avoid newline */
                    if (c != 10) {
                        m_mod_log(mod, "Unrecognized command. Beep. Please enter a new one... Totally not a bot.\n");
                    }
                    break;
            }
        }
    });
}
