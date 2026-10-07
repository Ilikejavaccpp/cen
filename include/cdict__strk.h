#ifdef __cplusplus
    extern "C" {
#endif

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

typedef struct {

} sdict;

#endif /* CEN__DICT__STRING_KEY_VARIANT_H */

#ifdef __cplusplus
    }
#endif
