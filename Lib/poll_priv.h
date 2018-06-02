#pragma once

#include <module_cmn.h>
#include <module_priv.h>

/* Useful macros to smooth away differences between supported OS */
enum op_type { ADD, RM };

int poll_create(void);
int poll_set_data(void **_ev, void *p);
int poll_set_new_evt(module_poll_t *tmp, m_context *c, enum op_type flag);
int poll_wait(int fd, int num_fds);
module_poll_t *poll_recv(int idx);
int poll_close(int fd);
