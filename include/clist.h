#ifdef __cplusplus
    extern "C" {
#endif

#ifndef CEN__CLIST_H
#define CEN__CLIST_H

#include "../include/ccompare.h"
#include "../include/calias.h"
#include "../include/cstr.h"
#include <stdlib.h>
#include <string.h>

typedef enum {
    CLIST_TYPE_INT = 0,
    CLIST_TYPE_FLOAT,
    CLIST_TYPE_DOUBLE,
    CLIST_TYPE_CHAR,
    CLIST_TYPE_STR,
    CLIST_TYPE_PTR
} clist_type_t;

typedef struct {
    clist_type_t type;
    union {
        int i;
        float f;
        double d;
        char c;
        char *s;
        void *p;
    } as;
} clist_item_t;

/* clist: stores data as contiguous memory of pointers: [P1] [P2] ... -> [D1], [D2] ... */
typedef struct {
    int size;
    int capacity;
    clist_item_t **data;
} clist;

/* clist_fast: stores data as contiguous memory of raw items: [D1] [D2] ... [Dend] */
typedef struct {
    int size;
    int capacity;
    clist_item_t *data;
} clist_fast;

typedef enum {
    CLIST_PRINT_ARRAY = 0,
    CLIST_PRINT_PRETTY,
    CLIST_PRINT_HEADING
} clist_print_mode_t;

/* --- clist API --- */
clist clist_init(void);
clist *clist_append_int(clist *list, int val);
clist *clist_append_float(clist *list, float val);
clist *clist_append_double(clist *list, double val);
clist *clist_append_char(clist *list, char val);
clist *clist_append_str(clist *list, const char *val);
clist *clist_append_ptr(clist *list, void *val);

clist_item_t *clist_at(const clist *list, int index);
clist_item_t **clist_next(clist_item_t **ptr); /* pointer math function */
clist clist_slice(const clist *list, int start, int stop, int step);
clist *clist_delete(clist *list, int index);
clist *clist_destroy(clist *list);
void clist_print(const clist *list, clist_print_mode_t mode);

_cmp_val clist_cmp(const clist *list, const clist *other);
_cmp_val clist_cmp__pointers_def(clist_item_t *self, clist_item_t *other);
#define clist_cmp__pointers_impl(self_ptr, other_ptr)

/* --- clist_fast API --- */
clist_fast clist_fast_init(void);
clist_fast *clist_fast_append_int(clist_fast *list, int val);
clist_fast *clist_fast_append_float(clist_fast *list, float val);
clist_fast *clist_fast_append_double(clist_fast *list, double val);
clist_fast *clist_fast_append_char(clist_fast *list, char val);
clist_fast *clist_fast_append_str(clist_fast *list, const char *val);
clist_fast *clist_fast_append_ptr(clist_fast *list, void *val);

clist_item_t *clist_fast_at(const clist_fast *list, int index);
clist_item_t *clist_fast_next(clist_item_t *ptr); /* pointer math function */
clist_fast clist_fast_slice(const clist_fast *list, int start, int stop, int step);
clist_fast *clist_fast_delete(clist_fast *list, int index);
clist_fast *clist_fast_destroy(clist_fast *list);
void clist_fast_print(const clist_fast *list, clist_print_mode_t mode);

#endif

#ifdef __cplusplus
    }
#endif
