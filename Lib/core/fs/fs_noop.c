#include "fs.h"
#include <errno.h>

int fs_init(m_ctx_t *c) {
    return -ENOSYS;
}

int fs_notify(m_mod_t *mod, const m_queue_t *const evts) {
    return -ENOSYS;
}

int fs_ctx_stopped(m_mod_t *mod) {
    return -ENOSYS;
}

int fs_cleanup(m_mod_t *mod) {
    return -ENOSYS;
}

int fs_end(m_ctx_t *c) {
    return -ENOSYS;
}
