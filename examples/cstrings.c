#define CEN__FLAG_OPTIMIZED

#include "../include/stden.h"

int main() {
    char string_c[] = "Hello, World!";
    printf("%s\n", string_c);

    str my_string = str_init(string_c);

    str_t string_arena;
    int offset = 0;
    str_initArena(&string_arena, &offset);

    printf("C STRING => %s\n", my_string.pointer);
    printf("Now let's change the string via appending `HIHIHI`\n");
    str_append(&my_string, "HIHIHI", &string_arena);
    printf("C STRING => %s\n", my_string.pointer);

    return 0;
}
