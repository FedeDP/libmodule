#pragma once

#include "mod.h"

int tell_system_pubsub_msg(const m_mod_t *recipient, m_ctx_t *c, m_mod_t *sender, const char *topic);
int flush_pubsub_msgs(void *data, const char *key, void *value);
void call_pubsub_cb(m_mod_t *mod, m_queue_t *evts);
