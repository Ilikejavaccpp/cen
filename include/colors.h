#ifdef __cplusplus
    extern "C" {
#endif

#ifndef CEN__COLORS_H
#define CEN__COLORS_H

#include "../include/cbool.h"
#include "../include/cstr.h"

#ifdef __cplusplus
    str hex_to_ansi(const char *hex, str_t *arena, bool bg);
#else
    str hex_to_ansi(const char hex[static 7], str_t *arena, bool bg);
#endif

#endif

#ifdef __cplusplus
    }
#endif
