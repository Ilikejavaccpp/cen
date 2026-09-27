#ifdef __cplusplus
    extern "C" {
#endif

#ifndef  CLIST_LINKEDLIST_C_H
#define  CLIST_LINKEDLIST_C_H

#include  <stdlib.h>
#include "../include/clinked_list.h"

void cllist_initMemory(cllist_pool *memory_pool) {
    cllist_mem *mem = (cllist_mem *)malloc(sizeof(cllist_mem));

    if  (!mem) return;

    mem->next_mem = NULL;
    memory_pool->slabsd = mem; /* initialize the memory pool slab with ~2KiB in RAM */

    /* if this was fixed, then we could've used a shitton of sets to not
     * waste time via a O(n) for-loop
     */
    for (int i = 0; i < CLLIST_MEMPOOL_SIZE - 1; i++) {
        mem->nodes[i].next = &mem->nodes[i + 1];
    }
    mem->nodes[CLLIST_MEMPOOL_SIZE - 1].next = NULL; /* same for the tail */
    memory_pool->avail = &mem->nodes[0]; /* initialize the free node list's head */
}

cllist_node_t *cllist_allocMemory(cllist_pool *pool) {
    if (!pool->avail) {
        cllist_initMemory(pool);
    }

    cllist_node_t *node = pool->avail;
    pool->avail = node->next;

    /* clean */
    node->prev = NULL;
    node->next = NULL;

    return node;
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
    cllist_node_t *node = cllist_allocMemory(list->pool);

    return NULL; /* remove the fucking warning */
}


#endif

#ifdef  __cplusplus
    }
#endif
