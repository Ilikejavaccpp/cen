#include <sys/cdefs.h>
#ifdef __cplusplus
    extern "C" {
#endif

#ifndef CLIST_LINKEDLIST_H
#define CLIST_LINKEDLIST_H

#include "../include/cbool.h"
#include "../include/ccompare.h"
#include "../include/calias.h"
#include "../include/cstr.h"
#include "../include/clist_fast.h"

#define CLLIST_NODE_SIZE 16 /* used for fast caching */
#define CLLIST_NODE_ALIGN 64 /* 64 bytes alignment */
#define CLLIST_MEMADDR_PTR void *
#define CLLIST_MEMPOOL_SIZE 128 /* 128 nodes or 128 * 16 bytes = 2 KiB. very enough,
                                  made it so that you can define this to fit your needs*/

/* --------------------------------------------------------------------------------- */

/* code here */
#ifndef CEN__FLAG_OPTIMIZED

typedef enum {
    CLIST_TYPE_INT = 0,
    CLIST_TYPE_FLOAT,
    CLIST_TYPE_DOUBLE,
    CLIST_TYPE_CHAR,
    CLIST_TYPE_STR,
    CLIST_TYPE_PTR,

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
void cllist_initMemory(cllist_pool * memory_pool);

cllist_node_t *cllist_allocMemory(cllist_pool *pool);

cllist * cllist_push(cllist *list, void *data, cllist_type_t type_of_data);


/* --------------------------------------------------------------------------------- */
#undef CLLIST_MEMADDR_PTR

#endif
#endif

#ifdef __cplusplus
}
#endif
