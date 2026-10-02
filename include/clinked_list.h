#include <sys/cdefs.h>
#ifdef __cplusplus
    extern "C" {
#endif

#ifndef CEN__CLIST_LINKEDLIST_H
#define CEN__CLIST_LINKEDLIST_H

#include "../include/cbool.h"
#include "../include/ccompare.h"
#include "../include/calias.h"
#include "../include/cstr.h"
#include "../include/clist_fast.h"

#include <stdint.h>

#define CLLIST_NODE_SIZE 16 /* used for fast caching */
#define CLLIST_NODE_ALIGN 64 /* 64 bytes alignment */
#define CLLIST_MEMADDR_PTR void *
#define CLLIST_MEMPOOL_SIZE 128 /* 128 nodes or 128 * 16 bytes = 32 KiB. very enough,
                                  made it so that you can define this to fit your needs*/

/* type casting macros */
#ifndef __cplusplus
    #define CLLIST_MEM_FLOAT(f) \
        ((void*)(uintptr_t)((union { f32 f_val; uintptr_t ui_val; }){ .ui_val = 0, .f_val = (f) }).ui_val)
    #define CLLIST_FLOAT(p) \
        (((union { f32 f_val; uintptr_t ui_val; }){ .ui_val = (uintptr_t)(p) }).f_val)

    #define CLLIST_MEM_DOUBLE(d) \
        ((void*)(uintptr_t)((union { f64 d_val; uintptr_t ui_val; }){ .ui_val = 0, .d_val = (d) }).ui_val)
    #define CLLIST_DOUBLE(p) \
        (((union { f64 d_val; uintptr_t ui_val; }){ .ui_val = (uintptr_t)(p) }).d_val)

    #define CLLIST_MEM_INT(i) \
        ((void*)(uintptr_t)((union { int i_val; uintptr_t ui_val; }){ .ui_val = 0, .i_val = (i) }).ui_val)
    #define CLLIST_INT(p) \
        (((union { int i_val; uintptr_t ui_val; }){ .ui_val = (uintptr_t)(p) }).i_val)

    #define CLLIST_MEM_CHAR(c) \
        ((void*)(uintptr_t)((union { char c_val; uintptr_t ui_val; }){ .ui_val = 0, .c_val = (c) }).ui_val)
    #define CLLIST_CHAR(p) \
    (((union { char c_val; uintptr_t ui_val; }){ .ui_val = (uintptr_t)(p) }).c_val)
#else
union __cllist_ptrconvert {
    void* p_val;
    uintptr_t ui_val;
    f32 f_val;
    f64 d_val;
    i32 i_val;
    char c_val;
};

#define CLLIST_MEM_FLOAT(f) (((union __cllist_ptrconvert){.f_val = (f)}).p_val)
#define CLLIST_MEM_DOUBLE(f) (((union __cllist_ptrconvert){.d_val = (f)}).p_val)
#define CLLIST_MEM_INT(i)   (((union __cllist_ptrconvert){.i_val = (i)}).p_val)
#define CLLIST_MEM_CHAR(c)  (((union __cllist_ptrconvert){.c_val = (c)}).p_val)

/* --- UNPACK MACROS --- */
#define CLLIST_FLOAT(p) (((union __cllist_ptrconvert){.p_val = (p)}).f_val)
#define CLLIST_DOUBLE(p) (((union __cllist_ptrconvert){.p_val = (p)}).d_val)
#define CLLIST_INT(p)   (((union __cllist_ptrconvert){.p_val = (p)}).i_val)
#define CLLIST_CHAR(p)  (((union __cllist_ptrconvert){.p_val = (p)}).c_val)

#endif

/* --------------------------------------------------------------------------------- */

/* code here */
#ifndef CEN__FLAG_OPTIMIZED

#endif

typedef enum {
    CLLIST_TYPE_INT = 0,
    CLLIST_TYPE_FLOAT,
    CLLIST_TYPE_DOUBLE,
    CLLIST_TYPE_CHAR,
    CLLIST_TYPE_STR,
    CLLIST_TYPE_PTR,

    /* enable support via the `cNAME_fast.h` standard */
    // CLIST_TYPE_LIST,
    // CLIST_TYPE_DICT,
    // CLIST_TYPE_ROLS, // tuple
} cllist_type_t;

typedef struct {
    cllist_type_t types[16];
    union {
        i32 i;
        f32 f;
        f64 d;
        char c;
        char *str;
        void *ptr;
    } as[16];

    /* use `void *` since it is a memory address literal */
    void * next;
    void * prev;

    i8 count; /* since everything is packed */
} __attribute__((aligned(CLLIST_NODE_ALIGN))) cllist_node_t;


/* the memory pool slab for clist nodes, i.e. where it lives contiguously,
 * this is still not considered fast as it doesn't operate on the stack */
typedef struct cllist_mem {
    struct cllist_mem * next_mem;
    cllist_node_t nodes[CLLIST_MEMPOOL_SIZE]; /* @default: 128 */
} cllist_mem;

/* the memory pool for clist nodes, i.e. where it lives contiguously,
 * this is still not considered fast as it doesn't operate on the stack.
 * It is a helper / manager wrapper for the memory pool slab (`cllist_mem`). */
typedef struct {
    cllist_mem *slabsd; /* SLABs uSeD for memory allocation (RAM) */
    cllist_node_t *avail; /* free nodes in the pool */
} cllist_pool;

typedef struct {
    cllist_node_t *head;/* head sentinel / marker */
    cllist_node_t *tail;/* tail sentinel / marker */
    u64 capacity; /* length, used by `cllist_len()` or `cllist_getcap()` */
    cllist_pool *pool; /* memory pool for this list */
} cllist;

cllist cllist_init(cllist_pool *memory_pool);
void cllist_initMemory(cllist_pool * memory_pool); /* initializes or grows, requires a zeroed pool to start */
void cllist_destroyMemory(cllist_pool * memory_pool); /* frees every slab, the pool is dead after this */

cllist_node_t *cllist_allocMemory(cllist_pool *pool);
void cllist_freeMemory(cllist_pool *pool, cllist_node_t *node); /* recycles one node, the pool stays usable */

cllist * cllist_push(cllist *list, void *data, cllist_type_t type_of_data);
cllist * cllist_insert(cllist *list, void *data, cllist_type_t type_of_data, u32 index); /* CHORE: change to `u64`if small */
cllist * cllist_shift(cllist *list); /* removes the head node and returns the new list */
cllist * cllist_unshift(cllist *list, void *data, cllist_type_t type_of_data); /* adds a node to the head of the list and returns the new list */

cllist * cllist_remove(cllist *list, u32 index);/* removes the node at given index and returns the new list */
cllist * cllist_pop(cllist *list); /* removes (pops) the tail node and returns the new list */

void cllist_print(cllist * list);

/* --------------------------------------------------------------------------------- */
#undef CLLIST_MEMADDR_PTR

#endif

#ifdef __cplusplus
}
#endif
