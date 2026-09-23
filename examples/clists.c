#define CEN__FLAG_OPTIMIZED

#include "../include/stden.h"

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

    return 0;
}
