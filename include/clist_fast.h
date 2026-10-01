#ifdef __cplusplus
    extern "C" {
#endif

#ifndef CEN__CLIST_FAST_H
#define CEN__CLIST_FAST_H

#ifdef CEN__FLAG_USE_OOP
    #include "../include/c__list_base.h"
#else

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

typedef enum {
    CLIST_PRINT_ARRAY = 0,
    CLIST_PRINT_PRETTY,
    CLIST_PRINT_HEADING
} clist_print_mode_t;

#endif


/* clist_fast: stores data as contiguous memory of raw items: [D1] [D2] ... [Dend] */
typedef struct {
    int size;
    int capacity;
    clist_item_t *data;
} clist_fast;


/* --- clist_fast API --- */
clist_fast clist_fast_init(void);
clist_fast *clist_fast_append_int(clist_fast *list, int val);
clist_fast *clist_fast_append_float(clist_fast *list, float val);
clist_fast *clist_fast_append_double(clist_fast *list, double val);
clist_fast *clist_fast_append_char(clist_fast *list, char val);
clist_fast *clist_fast_append_str(clist_fast *list, const char *val);
clist_fast *clist_fast_append_ptr(clist_fast *list, void *val);

clist_fast *clist_fast_insert_int(clist_fast *list, int val, int index);
clist_fast *clist_fast_insert_float(clist_fast *list, float val, int index);
clist_fast *clist_fast_insert_double(clist_fast *list, double val, int index);
clist_fast *clist_fast_insert_char(clist_fast *list, char val, int index);
clist_fast *clist_fast_insert_str(clist_fast *list, const char *val, int index);
clist_fast *clist_fast_insert_ptr(clist_fast *list, void *val, int index);

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
