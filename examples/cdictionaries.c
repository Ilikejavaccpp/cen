#define CEN__FLAG_OPTIMIZED

#include <stden.h>

int main() {
    /* define some elements */
    // 1. Hello World
    char *helloworld = "Hello World";

    // 2. Some greetings
    str hi = str_init("Hi");
    str hello = str_init("Hello");
    str general = str_init("How are you?");
    clist_fast greetings = clist_fast_init();

    clist_fast_append_str(&greetings, hi.pointer);
    clist_fast_append_str(&greetings, hello.pointer);
    clist_fast_append_str(&greetings, general.pointer);


    // 3. An address
    None *address = &hello;

    /* append them to a dictionary */
    dict my_dict = *dict_init();
    dict_set_str(&my_dict, "hw", helloworld);
    dict_set_ptr(&my_dict, "greetings", &greetings);
    dict_set_ptr(&my_dict, "hello", &address);
    dict_set_string(&my_dict, str_init("Hello"), hello);
    dict_set_strings(&my_dict, &(sstr){ "verity", 7 }, &(sstr){ "Verity™", 10});

    /* get the values back */
    // singular value
    const char *v;
    printf("%s[INFO]%s : Getting value `v` from key `k = %s`\n", "\033[35m", "\033[0m", "hw");
    dict_get_str(&my_dict, "hw", &v);
    printf("%s[INFO]%s : v = %s\n", "\033[35m", "\033[0m", v);

    printf("%s[INFO]%s : Getting value `v` from key `k = %s`\n", "\033[35m", "\033[0m", "hello");
    dict_get_str(&my_dict, "hello", &v);
    printf("%s[INFO]%s : v = %p\n", "\033[35m", "\033[0m", v);

    // plural value
    dict_print(&my_dict, DICT_PRINT_ARRAY);

    return 0;
}
