#ifdef __cplusplus
    extern "C" {
#endif

#ifndef  CLIST_LINKEDLIST_C_H
#define  CLIST_LINKEDLIST_C_H

#include  <stdlib.h>
#include "../include/clinked_list.h"

/* carves a fresh slab and threads its nodes onto the free list */
static cllist_mem *cllist_newSlab(void) {
    cllist_mem *mem = (cllist_mem *)malloc(sizeof(cllist_mem));

    if  (!mem) {
        return NULL;
    }

    mem->next_mem = NULL;

    /* if this was fixed, then we could've used a shitton of sets to not
     * waste time via a O(n) for-loop
     */
    for (int i = 0; i < CLLIST_MEMPOOL_SIZE - 1; i++) {
        mem->nodes[i].next = &mem->nodes[i + 1];
    }
    mem->nodes[CLLIST_MEMPOOL_SIZE - 1].next = NULL; /* same for the tail */
    return mem;
}

void cllist_initMemory(cllist_pool *memory_pool) {
    /* initialize or grow: a pool without slabs gets the first one, an exhausted
     * pool gets another appended. the slabsd chain stays intact either way, so
     * cllist_destroyMemory can still walk it later on. */
    cllist_mem *mem = cllist_newSlab();

    if  (!mem) {
        memory_pool->avail = NULL;
        return;
    }

    if (memory_pool->slabsd == NULL) {
        memory_pool->slabsd = mem; /* initialize the memory pool slab with ~2KiB in RAM */
    } else {
        cllist_mem *tail = memory_pool->slabsd;
        while (tail->next_mem != NULL) {
            tail = tail->next_mem;
        }
        tail->next_mem = mem;
    }

    memory_pool->avail = &mem->nodes[0]; /* initialize the free node list's head */
}

void cllist_destroyMemory(cllist_pool *memory_pool) {
    if (!memory_pool) return;

    cllist_mem *mem = memory_pool->slabsd;
    while (mem) {
        cllist_mem *next = mem->next_mem;
        free(mem);
        mem = next;
    }

    /* leave the pool reusable, same state cllist_initMemory tolerates */
    memory_pool->slabsd = NULL;
    memory_pool->avail = NULL;
}

cllist_node_t *cllist_allocMemory(cllist_pool *pool) {
    if (!pool->avail) {
        cllist_initMemory(pool);
        if (!pool->avail) return NULL;
    }

    cllist_node_t *node = pool->avail;
    pool->avail = node->next;

    /* clean */
    node->prev = NULL;
    node->next = NULL;
    node->count = 0;

    return node;
}

void cllist_freeMemory(cllist_pool *pool, cllist_node_t *node) {
    if (!node) return;

    // 1. Thread the node straight to the front of the available pool cache
    node->next = pool->avail;

    // 2. Move the head of the available nodes stack to this recycled node
    pool->avail = node;
}

cllist cllist_init(cllist_pool *memory_pool) {
    return (cllist){
        .head = NULL,
        .tail = NULL,
        .capacity = 0,
        .pool = memory_pool,
    };
}

cllist *cllist_push(cllist *list, void *data, cllist_type_t type_of_data) {
    cllist_node_t *node = list->tail;

    /* if we need to make a fresh one */
    if (!node || node->count == 16) {
        cllist_node_t *new_node = cllist_allocMemory(list->pool);
        if (!new_node) return list; /* no changes */

        new_node->prev = list->tail;
        new_node->next = NULL;

        if (list->tail) {
            ((cllist_node_t *)list->tail)->next = new_node;
        } else {
            list->head = new_node;
        }
        list->tail = new_node;
        node = new_node;
    }

    node->types[node->count] = type_of_data;

    switch (type_of_data) {
        case CLLIST_TYPE_INT: {
            node->as[node->count].i = CLLIST_INT(data);
            break;
        }
        case CLLIST_TYPE_FLOAT: {
            node->as[node->count].f = CLLIST_FLOAT(data);
            break;
        }
        case CLLIST_TYPE_DOUBLE: {
            node->as[node->count].d = CLLIST_DOUBLE(data);
            break;
        }
        case CLLIST_TYPE_CHAR: {
            node->as[node->count].c = CLLIST_CHAR(data);
            break;
        }
        case CLLIST_TYPE_STR: {
            node->as[node->count].str = (char *)data; /* since a `const char *` or cstr is a pointer */
            break;
        }
        case CLLIST_TYPE_PTR: {
            node->as[node->count].ptr = (void *)data;
            break;
        }
    }

    if (node->count < 16) node->count++;
    list->capacity++;

    return list;
}

cllist *cllist_insert(cllist *list, void *data, cllist_type_t type_of_data, u32 index) {
    /* Rule 1: If inserting at the very end, drop straight into the optimized push function */
    if (index == list->capacity) {
        return cllist_push(list, data, type_of_data);
    }
    if (index > list->capacity) return list; /* Out of bounds safety fallback */

    cllist_node_t *curr = (cllist_node_t *)list->head;
    u32 current_global_offset = 0;

    /* 1. Navigate to find which unrolled node houses this global index placement */
    while (curr) {
        if (index >= current_global_offset && index <= current_global_offset + curr->count) {
            break;
        }
        current_global_offset += curr->count;
        curr = (cllist_node_t *)curr->next;
    }

    if (!curr) return list; /* Fallback safety safety constraint */
    u32 internal_idx = index - current_global_offset;

    /* 2. Handle the Overflow Trap: If the node is maxed out, split it in half! */
    if (curr->count >= 16) {
        cllist_node_t *new_node = cllist_allocMemory(list->pool);
        if (!new_node) return list;

        /* Stitch the new node into the outer linked list topology right after 'curr' */
        new_node->next = curr->next;
        new_node->prev = curr;
        if (curr->next) {
            ((cllist_node_t *)curr->next)->prev = new_node;
        } else {
            list->tail = new_node;
        }
        curr->next = new_node;

        /* Move the latter half (last 8 elements) from 'curr' into the fresh 'new_node' */
        int split_point = 8;
        int move_count = curr->count - split_point;

        for (int i = 0; i < move_count; i++) {
            new_node->types[i] = curr->types[split_point + i];
            new_node->as[i]    = curr->as[split_point + i];
        }
        new_node->count = move_count;
        curr->count = split_point;

        /* Figure out if our target insertion point landed in the original node or the split node */
        if (internal_idx > split_point) {
            curr = new_node;
            internal_idx -= split_point;
        }
    }

    /* 3. Shift internal array elements right to create an opening for our data payload */
    for (int i = curr->count; i > (int)internal_idx; i--) {
        curr->types[i] = curr->types[i - 1];
        curr->as[i]    = curr->as[i - 1];
    }

    /* 4. Drop the value into the newly vacated array index slot */
    curr->types[internal_idx] = type_of_data;
    switch (type_of_data) {
        case CLLIST_TYPE_INT:    curr->as[internal_idx].i = CLLIST_INT(data);    break;
        case CLLIST_TYPE_FLOAT:  curr->as[internal_idx].f = CLLIST_FLOAT(data);  break;
        case CLLIST_TYPE_DOUBLE: curr->as[internal_idx].d = CLLIST_DOUBLE(data); break;
        case CLLIST_TYPE_CHAR:   curr->as[internal_idx].c = CLLIST_CHAR(data);   break;
        case CLLIST_TYPE_STR:    curr->as[internal_idx].str = (char *)data;      break;
        case CLLIST_TYPE_PTR:    curr->as[internal_idx].ptr = (void *)data;      break;
    }

    curr->count++;
    list->capacity++;

    return list;
}

cllist *cllist_remove(cllist *list, u32 index) {
    /* Safety constraint: index out of bounds */
    if (index >= list->capacity || !list->head) return list;

    cllist_node_t *curr = (cllist_node_t *)list->head;
    u32 current_global_offset = 0;

    /* 1. Navigate to the specific node housing the target index */
    while (curr) {
        if (index >= current_global_offset && index < current_global_offset + curr->count) {
            break; /* Target node identified */
        }
        current_global_offset += curr->count;
        curr = (cllist_node_t *)curr->next;
    }

    if (!curr) return list; /* Fallback fallback safety constraint */

    /* 2. Determine target offset inside the internal node array */
    u32 internal_idx = index - current_global_offset;

    /* 3. Shift remaining items inside the node array left to overwrite the index slot */
    for (int i = internal_idx; i < curr->count - 1; i++) {
        curr->types[i] = curr->types[i + 1];
        curr->as[i] = curr->as[i + 1];
    }

    /* Reduce indicators */
    curr->count--;
    list->capacity--;

    /* 4. Structural cleanup: If the node is completely empty, remove the outer wrapper */
    if (curr->count == 0) {
        cllist_node_t *prev_node = (cllist_node_t *)curr->prev;
        cllist_node_t *next_node = (cllist_node_t *)curr->next;

        if (prev_node) {
            prev_node->next = next_node;
        } else {
            list->head = next_node; /* Empty node was list head */
        }

        if (next_node) {
            next_node->prev = prev_node;
        } else {
            list->tail = prev_node; /* Empty node was list tail */
        }

        /* O(1) Fast Recycle back to the memory pool */
        cllist_freeMemory(list->pool, curr);
    }

    return list;
}

/* Define a punchy, clean function pointer type for printing variants */
typedef void (*cllist_print_fn)(const cllist_node_t *node, int slot);

/* Individual micro-inline printers with no conditional branching */
static inline void _print_int(const cllist_node_t *n, int s)    { printf("%d", n->as[s].i); }
static inline void _print_float(const cllist_node_t *n, int s)  { printf("%f", n->as[s].f); }
static inline void _print_double(const cllist_node_t *n, int s) { printf("%lf", n->as[s].d); }
static inline void _print_char(const cllist_node_t *n, int s)   { printf("'%c'", n->as[s].c); }
static inline void _print_str(const cllist_node_t *n, int s)    { printf("\"%s\"", n->as[s].str); }
static inline void _print_ptr(const cllist_node_t *n, int s)    { printf("%p", n->as[s].ptr); }

/*
 * The Branchless Jump Table: Maps cllist_type_t enums directly
 * to array index offsets for instant execution matching.
 */
static const cllist_print_fn print_table[] = {
    [CLLIST_TYPE_INT]    = _print_int,
    [CLLIST_TYPE_FLOAT]  = _print_float,
    [CLLIST_TYPE_DOUBLE] = _print_double,
    [CLLIST_TYPE_CHAR]   = _print_char,
    [CLLIST_TYPE_STR]    = _print_str,
    [CLLIST_TYPE_PTR]    = _print_ptr
};

void cllist_print(cllist *list) {
    if (!list || !list->head) {
        printf("[]\n");
        return;
    }

    const cllist_node_t *curr = (const cllist_node_t *)list->head;
    printf("[");

    /* 1. Extract and process the very first element manually to destroy the comma branch check */
    print_table[curr->types[0]](curr, 0);

    /* 2. Process the remaining elements inside the head node chunk */
    i8 head_count = curr->count;
    for (i8 i = 1; i < head_count; i++) {
        printf("%s - %s", "\033[1;32m", "\033[0m"); /* may also use arrows `->` or commas `, ` */
        print_table[curr->types[i]](curr, i);
    }

    curr = (const cllist_node_t *)curr->next;

    /* 3. Outer Loop: Deep execution block over subsequent full node segments */
    while (curr != NULL) {
        i8 count = curr->count;

        /* Inner Loop is now perfectly linear and highly branch-predictor friendly */
        for (i8 i = 0; i < count; i++) {
            printf("%s - %s", "\033[1;32m", "\033[0m"); /* may also use arrows `->` or commas `, ` */
            /* Zero switches: Direct arithmetic lookup into the function table */
            print_table[curr->types[i]](curr, i);
        }

        curr = (const cllist_node_t *)curr->next;
    }

    printf("] (len: %lu)\n", (u64)list->capacity);
}

#endif

#ifdef  __cplusplus
    }
#endif
