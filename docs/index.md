# Welcome to Libodule Documentation

Libmodule offers a small and simple C implementation of an actor library that aims to let developers easily create modular C projects in a way which is both simple and elegant.  

---

Module is the core entity of libmodule architecture.  
A module is an Actor that can listen on non-pubsub events too.  

Here is a short example of a module:
```C
#include <module/mod_easy.h>
#include <module/ctx.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

M_MOD("Pippo");

static bool m_mod_on_eval(mod_t *mod) {
    /* Should module be started? */
    return true;
}

static bool m_mod_on_start(mod_t *mod) {
    /* Register STDIN fd */
    m_mod_src_register(mod, STDIN_FILENO, 0, NULL);
    return true;
}

static void m_mod_on_stop(mod_t *mod) {
    
}

static void m_mod_on_evt(m_mod_t *mod, const m_queue_t *const evts) {
    m_itr_foreach(evts, {
        m_evt_t *msg = m_itr_get(m_itr);
        switch (msg->type) {
        case M_SRC_TYPE_FD: {
            char c;
            read(msg->fd_evt->fd, &c, sizeof(char));
            switch (tolower(c)) {
                case 'q':
                    m_mod_log(mod, "Leaving...\n");
                    m_mod_tell(mod, mod, "ByeBye", 0);
                    break;
                default:
                    if (c != ' ' && c != '\n') {
                        m_mod_log(mod, "Pressed '%c'\n", c);
                    }
                    break;
            }
            break;
        }
        case M_SRC_TYPE_PS:
            if (strcmp((char *)msg->ps_evt->data, "ByeBye") == 0) {
                m_mod_log("Bye\n"):
                m_ctx_quit(m_mod_ctx(mod), 0);
            }
            break;
        }
    }
}
```
