#define CEN__FLAG_OPTIMIZED

#include "../include/stden.h"
#include "../include/clinked_list.h"

#include <iostream>


int main() {
    /* first define our strings... */
    str str1 = str_init("hello");
    str str2 = str_init("world");

    /* creating an arena for all these strings
     * is optional since we are ONLY going to store them */

    /* the list */
    clist list = clist_init();
    clist_append_str(&list, str1.pointer);
    clist_append_str(&list, str2.pointer);

    clist_print(&list, CLIST_PRINT_PRETTY);

    /* now make a (Unrolled) linked list on the heap */
    cllist_pool pool = {}; /* must start zeroed: cllist_initMemory reads slabsd to tell init from grow */
    cllist_initMemory(&pool);

    std::cout << "CPP" << ", " << "\n";
    cllist list2 = cllist_init(&pool);

    /* append some elements of differnet types using one method */
    cllist_push(&list2, str1.pointer, cllist_type_t::CLLIST_TYPE_STR);
    cllist_push(&list2, CLLIST_MEM_CHAR (str2.pointer[3]), cllist_type_t::CLLIST_TYPE_CHAR);
    cllist_push(&list2, CLLIST_MEM_CHAR (str2.pointer[3]), cllist_type_t::CLLIST_TYPE_CHAR);
    cllist_push(&list2, CLLIST_MEM_CHAR (str2.pointer[3]), cllist_type_t::CLLIST_TYPE_CHAR);
    cllist_push(&list2, CLLIST_MEM_CHAR (str2.pointer[3]), cllist_type_t::CLLIST_TYPE_CHAR);
    cllist_push(&list2, CLLIST_MEM_CHAR (str2.pointer[3]), cllist_type_t::CLLIST_TYPE_CHAR);
    cllist_push(&list2, CLLIST_MEM_CHAR (str2.pointer[3]), cllist_type_t::CLLIST_TYPE_CHAR);
    cllist_push(&list2, (void *)("Bye"), cllist_type_t::CLLIST_TYPE_STR);

    cllist_insert(&list2, str2.pointer, cllist_type_t::CLLIST_TYPE_STR, 4);

    cllist_print(&list2);

    return 0;
}
