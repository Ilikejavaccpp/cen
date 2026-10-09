#define CEN__FLAG_OPTIMIZED

#include "../include/stden.h"
#include <iostream>

int main() {
    /* plan */

    // pythonic dict we are aiming at
    // d: dict = {
    //     "greet": "hello",
    //     "end": "goodbye"
    // }

    /* create the dictionary */
    dict *d = dict_init();
    dict_setPair(
        d,
        DICT_KEY_STRING("greet"),
        DICT_MEM_PTR("hello"),
        DICT_TYPE_STR
    );
    dict_setPair(
        d,
        DICT_KEY_STRING("end"),
        DICT_MEM_PTR("goodbye"),
        DICT_TYPE_STR
    );

    std::cout << "Length of pythonic CEN dictionary `d`: " << dict_len(d) << "\n";
    std::cout << "Printing dictionary `d`...\n";
    dict_print(d, DICT_PRINT_PRETTY);

    return 0;
}
