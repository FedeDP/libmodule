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

/*
 * Const-after-init struct members 
 * are marked by this macro 
 */
#define CONST 

#define BILLION 1000000000

#define M_MEM_LOCK(mem, func) \
    m_mem_ref(mem); \
    func; \
    m_mem_unref(mem);

void mem_dtor(void *src);
