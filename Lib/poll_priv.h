#pragma once

#include "priv.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>

/* Useful macros to smooth away differences between supported OS */
enum op_type { ADD, RM };

mod_ret poll_create(poll_priv_t *priv);
int poll_set_new_evt(poll_priv_t *priv, ev_src_t *tmp, const enum op_type flag);
mod_ret poll_init(poll_priv_t *priv);
int poll_wait(poll_priv_t *priv, const int timeout);
ev_src_t *poll_recv(poll_priv_t *priv, const int idx);
int poll_get_fd(poll_priv_t *priv);
mod_ret poll_clear(poll_priv_t *priv);
mod_ret poll_destroy(poll_priv_t *priv);
