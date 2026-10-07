#include "../include/stden.h"
#include <iostream>

int main() {
    cen::string my_str = "Hi";
    str_t arena;
    int offset = 0;
    str_initArena(&arena, &offset);
    std::cout << my_str.pointer << std::endl;
    my_str.append("Hello", &arena)->change("c", 4, &arena)->prints(stdout);

    return 0;
}
