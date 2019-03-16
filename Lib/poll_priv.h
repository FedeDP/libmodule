#pragma once

#include "priv.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>

/* Useful macros to smooth away differences between supported OS */
enum op_type { ADD, RM };

int poll_create(void);
int poll_set_data(void **_ev);
int poll_set_new_evt(module_poll_t *tmp, m_context *c, const enum op_type flag);
int poll_init_pevents(void **pevents, const int max_events);
int poll_wait(const int fd, const int max_events, void *pevents, const int timeout);
module_poll_t *poll_recv(const int idx, void *pevents);
int poll_destroy_pevents(void **pevents, int *max_events);
int poll_close(const int fd, void **pevents, int *max_events);
