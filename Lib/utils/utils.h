#pragma once

#include <stdint.h>
#include <stdbool.h>

void fetch_ms(uint64_t *val, uint64_t *ctr);
bool str_not_empty(const char *str);
