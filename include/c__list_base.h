#ifdef __cplusplus
extern "C" {
#endif

#ifndef CEN__CLIST_BASE_H
#define CEN__CLIST_BASE_H

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

#ifdef __cplusplus
}
#endif
