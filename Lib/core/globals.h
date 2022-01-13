#pragma once

#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <inttypes.h> // PRIu64
#include <stdarg.h>
#include <unistd.h>
#include "public/module/structs/itr.h"
#include "public/module/mem/mem.h"
#include "public/module/ctx.h"
#include "utils.h"
#include "mem.h"
#include "log.h"

// Const-after-init struct members are outlined by this
#define CONST 

#define M_MEM_LOCK(mem, func) \
    m_mem_ref(mem); \
    func; \
    m_mem_unref(mem);

void mem_dtor(void *src);

/* Global variables are defined in main.c */
extern m_map_t *ctx;
extern pthread_mutex_t mx;          // Used to access/modify global ctx map
extern m_ctx_t *default_ctx;
