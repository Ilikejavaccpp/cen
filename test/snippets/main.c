/* -- This is a real life example of how you can utilize CEN via
 * - safe strings,
 * - awesome lists,
 * - nice color formatting,
 * - the ok math library */
#ifdef __cplusplus
    extern "C" {
#endif

#include <stden.h>
#include <stdio.h>

#include <sqlite3.h>

#define COLOR_RESET "\033[0m"
#define COLOR_INFOX "#87daf6"
/* ------------------------------------------------------------------------------------------ */

str_t arena;
int offset = 0;
i32 id = 2;

void add_item(clist *member_row, str name, str variable_name, str description,
    i16 position_x, i16 position_y, str parent, i32 parent_id, i32 *_id)
{
    if (_id == NULL) clist_append_int(member_row, id);
    else clist_append_int(member_row, *_id);
    clist_append_str(member_row, name.pointer);
    clist_append_str(member_row, variable_name.pointer);
    clist_append_str(member_row, description.pointer);
    clist_append_int(member_row, position_x);
    clist_append_int(member_row, position_y);
    clist_append_str(member_row, parent.pointer);
    clist_append_int(member_row, parent_id);
    *_id += 1;
}

int main() {
    /* initialization */
    str_initArena(&arena, &offset); /* null since *null --> *0 == 0 */

    /* first, let's get our data. */
    printf("%s[INFO]%s : Getting data...\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET);

    /* elements GTK
     * - Calculator Display
     * - Number Buttons ( 0 - 9 )
     * - Equals Button
     * - Clear Button
     * - Basic Arithmetic Buttons ( +, -, *, /, (, ) )
     */
#pragma region GTkElements
    clist calculator_display = clist_init();
    add_item(&calculator_display,
        (str){"GtkWidget", .length=9}, // 1
        (str){"display", .length=11}, // 2
        (str){"Calculator display", .length=19}, // 3
        10, 10,             // 4, 5
        (str){"box", 3}, 0,  // 6, 7
        &id);

    /* Number Buttons ( 0 - 9 )
     * - positions mirror @cbp's RAYLIB calculator, 400x600 window:
     *   - margin 16, display height 100, gap 10
     *   - topOffset = 16 + 100 + 16 = 132
     *   - btnWidth  = (368 - 3 * 10) / 4 = 84.5  -> columns at 16, 110.5, 205, 299.5
     *   - btnHeight = (452 - 4 * 10) / 5 = 82.4  -> rows at 132, 224.4, 316.8, 409.2, 501.6
     *   - numpad grid is the 3x4 block left of the 4th (operator) column
     */
    clist_fast calculator_numpad = clist_fast_init();
    {
        clist numpad_1 = clist_init();
        add_item(&numpad_1,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button1", .length=6}, // 2
            (str){"Calculator button numpad `1`", .length=29}, // 3
            16, 409,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_numpad, &numpad_1);
    }
    {
        clist numpad_2 = clist_init();
        add_item(&numpad_2,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button2", .length=6}, // 2
            (str){"Calculator button numpad `2`", .length=29}, // 3
            110, 409,           // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_numpad, &numpad_2);
    }
    {
        clist numpad_3 = clist_init();
        add_item(&numpad_3,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button3", .length=6}, // 2
            (str){"Calculator button numpad `3`", .length=29}, // 3
            205, 409,           // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_numpad, &numpad_3);
    }
    {
        clist numpad_4 = clist_init();
        add_item(&numpad_4,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button4", .length=6}, // 2
            (str){"Calculator button numpad `4`", .length=29}, // 3
            16, 317,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_numpad, &numpad_4);
    }
    {
        clist numpad_5 = clist_init();
        add_item(&numpad_5,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button5", .length=6}, // 2
            (str){"Calculator button numpad `5`", .length=29}, // 3
            110, 317,           // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);

        clist_fast_append_ptr(&calculator_numpad, &numpad_5);
    }
    {
        clist numpad_6 = clist_init();
        add_item(&numpad_6,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button6", .length=6}, // 2
            (str){"Calculator button numpad `6`", .length=29}, // 3
            205, 317,           // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_numpad, &numpad_6);
    }
    {
        clist numpad_7 = clist_init();
        add_item(&numpad_7,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button7", .length=6}, // 2
            (str){"Calculator button numpad `7`", .length=29}, // 3
            16, 224,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_numpad, &numpad_7);
    }
    {
        clist numpad_8 = clist_init();
        add_item(&numpad_8,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button8", .length=6}, // 2
            (str){"Calculator button numpad `8`", .length=29}, // 3
            110, 224,           // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_numpad, &numpad_8);
    }
    {
        clist numpad_9 = clist_init();
        add_item(&numpad_9,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button9", .length=6}, // 2
            (str){"Calculator button numpad `9`", .length=29}, // 3
            205, 224,           // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_numpad, &numpad_9);
    }
    {
        clist numpad_0 = clist_init();
        add_item(&numpad_0,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button0", .length=6}, // 2
            (str){"Calculator button numpad `0`", .length=29}, // 3
            16, 501,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_numpad, &numpad_0);
    }

    /* Operator buttons: + - * / */
    clist_fast calculator_operators = clist_fast_init();
    {
        clist operator_divide = clist_init();
        add_item(&operator_divide,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_div", .length=11}, // 2
            (str){"Calculator button operator `/`", .length=28}, // 3
            300, 130,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &operator_divide);
    } /* `/` */
    {
        clist operator_divide = clist_init();
        add_item(&operator_divide,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_mul", .length=11}, // 2
            (str){"Calculator button operator `/`", .length=28}, // 3
            394, 130,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &operator_divide);
    } /* `*` */
    {
        clist operator_divide = clist_init();
        add_item(&operator_divide,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_divide", .length=11}, // 2
            (str){"Calculator button operator `/`", .length=28}, // 3
            489, 130,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &operator_divide);
    } /* `-` */
    {
        clist operator_divide = clist_init();
        add_item(&operator_divide,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_add", .length=11}, // 2
            (str){"Calculator button operator `/`", .length=28}, // 3
            584, 130,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &operator_divide);
    } /* `+` */

#pragma endregion GTkElements

    // TODO: add more GTK UI elements relative / in accordance to @cbp's RAYLIB GTK CALCULATOR

    cllist_pool memory;
    cllist_initMemory(&memory);

    cllist my_list = cllist_init(&memory);

    cllist_push(&my_list, &calculator_display, CLLIST_TYPE_PTR); /* points to the calculator_display list, may access the others */
    cllist_print(&my_list);
    clist_print(&calculator_display, CLIST_PRINT_PRETTY);

    printf("%s[INFO]%s : Printing GTK metadata of %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, "display");
    printf("%s[INFO]%s :    ID --> %d\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, clist_at(&calculator_display, 0)->as.i);
    printf("%s[INFO]%s :    POS -> { %d, %d }\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET,
            clist_at(&calculator_display, 4)->as.i,
            clist_at(&calculator_display, 5)->as.i);


    printf("%s[INFO]%s : Printing GTK metadata of %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, "numpad");
    for (int i = 0; i < calculator_numpad.size; ++i) {
        clist *numpad_i = (clist *)clist_fast_at(&calculator_numpad, i)->as.p;
        printf("%s......%s : Printing GTKWidget `button` numpad button %s metdata\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, clist_at(numpad_i, 2)->as.s);
        clist_print(numpad_i, CLIST_PRINT_ARRAY);
    }

    printf("%s[INFO]%s : Printing GTK metadata of %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, "operators");
    for (int i = 0; i < calculator_operators.size; ++i) {
        clist *operator_i = (clist *)clist_fast_at(&calculator_operators, i)->as.p;
        printf("%s......%s : Printing GTKWidget `button` operator %s metdata\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, clist_at(operator_i, 2)->as.s);
        clist_print(operator_i, CLIST_PRINT_ARRAY);
    }

    // printf("%s[INFO]%s : Printing size vs. capacity...\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET);
    // printf("%d / %d (mul of 4)\n", calculator_numpad.size, calculator_numpad.capacity);

    printf("%s[INFO]%s : Freeing memory...\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET);

    for (int i = 0; i < calculator_numpad.size; ++i) {
        clist_destroy((clist *)clist_fast_at(&calculator_numpad, i)->as.p);
    }
    clist_fast_destroy(&calculator_numpad);
    cllist_freeMemory(&memory, my_list.head);


    return 0;
}

/* ------------------------------------------------------------------------------------------ */

#ifdef  __cplusplus
    }
#endif
