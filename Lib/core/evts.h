#pragma once

#include "src.h"

/* Struct that holds an event + its source, private */
typedef struct _ev_priv {
    m_evt_t evt;
    ev_src_t *src;                          // Ref to src that caused the event
} evt_priv_t;

evt_priv_t *new_evt(ev_src_t *src);

