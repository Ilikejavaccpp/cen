#ifdef __cplusplus
    extern "C" {
#endif

// TODO: fix the overwrite in @file `src/cdict.c` @func `dict_setPair()`

#ifndef CEN__DICT_H
    #error "ERROR: `cdict__strk.h` should be included after `cdict.h`"
#endif

/* There are no other variants, hence why we call it generic `dict` */
#ifndef CEN__DICT__STRING_KEY_VARIANT_H
#define CEN__DICT__STRING_KEY_VARIANT_H 1

/* THIS SHOULD BE INCLUDED AFTER THE MAIN DICT HEADER */
#define CEN__DICT_VERSION 20261002L // since this is also a dict
#define CEN__DICT_VERSION__UNSTABLE 20261007L

#include "../include/cbool.h"
#include "../include/calias.h"
#include "../include/cstr.h"

/* --- tunables --- */
#ifndef DICT_MIN_CAPACITY
#define DICT_MIN_CAPACITY 16u
#endif

#ifndef DICT_LOAD_NUM
#define DICT_LOAD_NUM 7u
#endif
#ifndef DICT_LOAD_DEN
#define DICT_LOAD_DEN 10u
#endif

#ifndef DICT_BUCKET_RATIO
#define DICT_BUCKET_RATIO 2u
#endif

#ifndef DICT_ENTRY_ALIGN
#define DICT_ENTRY_ALIGN 64u
#endif

#ifndef DICT_STR_HEADER
#define DICT_STR_HEADER 8u
#endif

#ifndef DICT_STR_ALIGN
#define DICT_STR_ALIGN 8u
#endif

#ifndef DICT_SLAB_SIZE
#define DICT_SLAB_SIZE 4096u
#endif

/* use these for STRing Key variants */
#define DICT_KEY_INT(integer) \
    { .integer = DICT_MEM_INT(integer), .as = 1 }
#define DICT_KEY_STRING(string) \
    { .pointer = (const char *)DICT_MEM_PTR(string), .as = 0 }

#ifndef CEN__DICT_H
typedef struct dict dict; /* suppress lsp errors */
typedef enum {
    DICT_TYPE_NONE = 0, /* slot is unused or a tombstone */
    DICT_TYPE_INT,
    DICT_TYPE_FLOAT,
    DICT_TYPE_DOUBLE,
    DICT_TYPE_CHAR,
    DICT_TYPE_STR, /* bytes are owned by the arena, never a borrowed pointer */
    DICT_TYPE_PTR
} dict_type_t;

#endif

typedef struct {
    union {
        const char * pointer;
        void * integer;
    };
    bool as; // we can utilize it as a bit
} dict_key_t;

dict * dict_setPair(
    dict * self,
    dict_key_t key,
    // whether to put val here
    void * val, // may make ts an object (wrapped) for safety, wait.. ts allows addresses as v, good
    dict_type_t vtype
);

#endif /* CEN__DICT__STRING_KEY_VARIANT_H */

#ifdef __cplusplus
    }
#endif
