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

/* U+232B ERASE TO THE LEFT: the standard backspace/delete glyph, NFC form */
#define ICON_BACKSPACE "\u232B"

#define DB_PATH    "data/data.dat"
#define SCHEMA_PATH "data/base.sql"
#define WIDGET_INSERT \
    "INSERT OR REPLACE INTO widgets " \
    "(id, name, variable_name, description, position_x, position_y, parent, parent_id) " \
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
/* ------------------------------------------------------------------------------------------ */

str_t arena;
int offset = 0;
/* base.sql already owns ids 1 (window) and 2 (box), so the calculator starts at 3 */
i32 id = 3;

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

/* slurps a whole file into a malloc'd NUL-terminated buffer, or NULL on failure */
char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return NULL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    if (size < 0) { fclose(file); return NULL; }

    char *buffer = malloc((size_t)size + 1);
    if (buffer == NULL) { fclose(file); return NULL; }

    size_t read = fread(buffer, 1, (size_t)size, file);
    buffer[read] = '\0';
    fclose(file);
    return buffer;
}

/* SQLITE_TRANSIENT makes sqlite copy the text, so the CEN row may be freed right after */
void bind_text_or_null(sqlite3_stmt *stmt, int index, char *value) {
    if (value == NULL) sqlite3_bind_null(stmt, index);
    else sqlite3_bind_text(stmt, index, value, -1, SQLITE_TRANSIENT);
}

/* appends one clist row (the add_item field order) into the widgets table */
void insert_widget(sqlite3 *db, sqlite3_stmt *stmt, clist *row) {
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    sqlite3_bind_int(stmt, 1, clist_at(row, 0)->as.i);
    bind_text_or_null(stmt, 2, clist_at(row, 1)->as.s);
    bind_text_or_null(stmt, 3, clist_at(row, 2)->as.s);
    bind_text_or_null(stmt, 4, clist_at(row, 3)->as.s);
    sqlite3_bind_int(stmt, 5, clist_at(row, 4)->as.i);
    sqlite3_bind_int(stmt, 6, clist_at(row, 5)->as.i);
    bind_text_or_null(stmt, 7, clist_at(row, 6)->as.s);
    sqlite3_bind_int(stmt, 8, clist_at(row, 7)->as.i);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("  [WARN] insert failed: %s\n", sqlite3_errmsg(db));
    }
}

int main(int argc, char **argv) {
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
        16, 16,             // 4, 5  -> top-left, just under the window margin
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
    /* hoisted out of the {} blocks: these must outlive the append, otherwise
     * calculator_numpad would hold pointers to dead stack slots */
    clist numpad_1, numpad_2, numpad_3, numpad_4, numpad_5;
    clist numpad_6, numpad_7, numpad_8, numpad_9, numpad_0;
    {
        numpad_1 = clist_init();
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
        numpad_2 = clist_init();
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
        numpad_3 = clist_init();
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
        numpad_4 = clist_init();
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
        numpad_5 = clist_init();
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
        numpad_6 = clist_init();
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
        numpad_7 = clist_init();
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
        numpad_8 = clist_init();
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
        numpad_9 = clist_init();
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
        numpad_0 = clist_init();
        add_item(&numpad_0,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button0", .length=6}, // 2
            (str){"Calculator button numpad `0`", .length=29}, // 3
            16, 501,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_numpad, &numpad_0);
    }

    /* Arithmetic Operators: / * - + -> 4th column of the grid
     * - `/` -> { 300, 132 }
     * - `*` -> { 300, 224 }
     * - `-` -> { 300, 317 }
     * - `+` -> { 300, 409 }
     */
    clist_fast calculator_operators = clist_fast_init();
    clist operator_div, operator_mul, operator_sub, operator_add;
    clist button_clear, button_lparen, button_rparen;
    clist button_dot, button_equals, button_backspace;
    {
        operator_div = clist_init();
        add_item(&operator_div,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_div", .length=10}, // 2
            (str){"Calculator button operator `/`", .length=29}, // 3
            300, 132,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &operator_div);
    } /* `/` */
    {
        operator_mul = clist_init();
        add_item(&operator_mul,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_mul", .length=10}, // 2
            (str){"Calculator button operator `*`", .length=29}, // 3
            300, 224,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &operator_mul);
    } /* `*` */
    {
        operator_sub = clist_init();
        add_item(&operator_sub,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_sub", .length=10}, // 2
            (str){"Calculator button operator `-`", .length=29}, // 3
            300, 317,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &operator_sub);
    } /* `-` */
    {
        operator_add = clist_init();
        add_item(&operator_add,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_add", .length=10}, // 2
            (str){"Calculator button operator `+`", .length=29}, // 3
            300, 409,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &operator_add);
    } /* `+` */

    /* Clear Button -> { 16, 132 } (top-left, above the `7`) */
    {
        button_clear = clist_init();
        add_item(&button_clear,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_clear", .length=12}, // 2
            (str){"Calculator button clear `C`", .length=26}, // 3
            16, 132,             // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &button_clear);
    } /* `C` */

    /* Parenthesis Buttons ( ) -> top row, flanking the display
     * - `(` -> { 110, 132 }
     * - `)` -> { 205, 132 }
     */
    {
        button_lparen = clist_init();
        add_item(&button_lparen,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_lparen", .length=13}, // 2
            (str){"Calculator button paren `(`", .length=26}, // 3
            110, 132,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &button_lparen);
    } /* `(` */
    {
        button_rparen = clist_init();
        add_item(&button_rparen,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_rparen", .length=13}, // 2
            (str){"Calculator button paren `)`", .length=26}, // 3
            205, 132,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &button_rparen);
    } /* `)` */

    /* Decimal Point -> { 110, 501 } (bottom row, right of `0`) */
    {
        button_dot = clist_init();
        add_item(&button_dot,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_dot", .length=10}, // 2
            (str){"Calculator button decimal `.`", .length=28}, // 3
            110, 501,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &button_dot);
    } /* `.` */

    /* Equals Button -> { 300, 501 } (bottom-right corner) */
    {
        button_equals = clist_init();
        add_item(&button_equals,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_equals", .length=13}, // 2
            (str){"Calculator button equals `=`", .length=27}, // 3
            300, 501,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &button_equals);
    } /* `=` */

    /* Backspace Button -> { 205, 501 } (bottom row, left of `=`)
     * - icon is U+232B ERASE TO THE LEFT, the standard single-glyph
     *   backspace/delete symbol (NFC, not ASCII art)
     */
    {
        button_backspace = clist_init();
        add_item(&button_backspace,
            (str){"GtkWidget", .length=9}, // 1
            (str){"button_backspace", .length=16}, // 2
            (str){"Calculator button backspace `" ICON_BACKSPACE "`", .length=32}, // 3
            205, 501,            // 4, 5
            (str){"box", 3}, 0,  // 6, 7
            &id);
        clist_fast_append_ptr(&calculator_operators, &button_backspace);
    } /* `⌫` */

#pragma endregion GTkElements

#pragma region SQLStore
    /* the CEN lists are the source of truth in RAM, the base database is where
     * they get appended. base.sql IS the base database: when data.dat has no
     * widgets table yet we execute base.sql to lay down the schema plus the
     * window/box rows, then append every list row on top. INSERT OR REPLACE
     * keeps re-runs idempotent. used once, so a bare scope holds the handles. */
    {
        sqlite3 *db = NULL;
        if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
            printf("%s[ERROR]%s : cannot open %s --> %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, DB_PATH, sqlite3_errmsg(db));
            sqlite3_close(db);
            return 1;
        }

        /* does the base database already carry the schema? */
        bool has_schema = false;
        sqlite3_stmt *probe = NULL;
        if (sqlite3_prepare_v2(db, "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'widgets'", -1, &probe, NULL) == SQLITE_OK) {
            has_schema = sqlite3_step(probe) == SQLITE_ROW;
            sqlite3_finalize(probe);
        }

        if (!has_schema) {
            char *script = read_file(SCHEMA_PATH);
            if (script == NULL) {
                printf("%s[ERROR]%s : cannot read %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, SCHEMA_PATH);
                sqlite3_close(db);
                return 1;
            }
            char *err = NULL;
            if (sqlite3_exec(db, script, NULL, NULL, &err) != SQLITE_OK) {
                printf("%s[ERROR]%s : base.sql failed --> %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, err ? err : "unknown");
                sqlite3_free(err);
                free(script);
                sqlite3_close(db);
                return 1;
            }
            free(script);
            printf("%s[INFO]%s : seeded the base database from %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, SCHEMA_PATH);
        }

        /* wipe any widget appended by a previous run before re-adding ours, so
         * the table always converges to exactly the widgets in the lists.
         * INSERT OR REPLACE alone cannot do this: rows left over under a stale
         * id would survive forever. window/box are not 'GtkWidget', so the
         * base rows stay untouched. */
        if (sqlite3_exec(db, "DELETE FROM widgets WHERE name = 'GtkWidget'", NULL, NULL, NULL) != SQLITE_OK) {
            printf("%s[ERROR]%s : cannot clear old widgets --> %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, sqlite3_errmsg(db));
            sqlite3_close(db);
            return 1;
        }

        sqlite3_stmt *insert = NULL;
        if (sqlite3_prepare_v2(db, WIDGET_INSERT, -1, &insert, NULL) != SQLITE_OK) {
            printf("%s[ERROR]%s : cannot prepare insert --> %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, sqlite3_errmsg(db));
            sqlite3_close(db);
            return 1;
        }

        int stored = 0;
        insert_widget(db, insert, &calculator_display);
        stored += 1;

        for (int i = 0; i < calculator_numpad.size; ++i) {
            insert_widget(db, insert, (clist *)clist_fast_at(&calculator_numpad, i)->as.p);
            stored += 1;
        }
        for (int i = 0; i < calculator_operators.size; ++i) {
            insert_widget(db, insert, (clist *)clist_fast_at(&calculator_operators, i)->as.p);
            stored += 1;
        }

        sqlite3_finalize(insert);
        sqlite3_close(db);

        printf("%s[INFO]%s : appended %d widgets into %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, stored, DB_PATH);
    }
#pragma endregion SQLStore

#pragma region SQLRead
    /* read the database back and print rows.
     * pass a limit of 0 to print the entire table, e.g. ./bin/main 0 */
    int read_limit = 2;
    if (argc > 1) read_limit = atoi(argv[1]);

    {
        sqlite3 *db = NULL;
        if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
            printf("%s[ERROR]%s : cannot open %s --> %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, DB_PATH, sqlite3_errmsg(db));
            sqlite3_close(db);
            return 1;
        }

        /* LIMIT -1 is sqlite's "no limit", so 0 means the whole table here */
        char *sql = sqlite3_mprintf(
            "SELECT * FROM widgets ORDER BY id LIMIT %d", read_limit > 0 ? read_limit : -1);

        sqlite3_stmt *stmt = NULL;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            printf("%s[ERROR]%s : cannot prepare --> %s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, sqlite3_errmsg(db));
            sqlite3_free(sql);
            sqlite3_close(db);
            return 1;
        }

        int columns = sqlite3_column_count(stmt);
        printf("%s[INFO]%s : %d column(s): ", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, columns);
        for (int c = 0; c < columns; ++c) {
            printf("%s%s", sqlite3_column_name(stmt, c), c + 1 < columns ? ", " : "\n");
        }

        int row = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            row += 1;
            printf("%s......%s : row %d", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, row);
            for (int c = 0; c < columns; ++c) {
                /* sqlite3_column_type tells NULL apart from the empty string */
                switch (sqlite3_column_type(stmt, c)) {
                    case SQLITE_NULL:
                        printf(" %s=NULL", sqlite3_column_name(stmt, c));
                        break;
                    case SQLITE_INTEGER:
                        printf(" %s=%lld", sqlite3_column_name(stmt, c), (long long)sqlite3_column_int64(stmt, c));
                        break;
                    case SQLITE_FLOAT:
                        printf(" %s=%f", sqlite3_column_name(stmt, c), sqlite3_column_double(stmt, c));
                        break;
                    case SQLITE_BLOB:
                        printf(" %s=<%d bytes>", sqlite3_column_name(stmt, c), sqlite3_column_bytes(stmt, c));
                        break;
                    default:
                        printf(" %s=%s", sqlite3_column_name(stmt, c), (const char *)sqlite3_column_text(stmt, c));
                        break;
                }
            }
            printf("\n");
        }

        sqlite3_finalize(stmt);
        sqlite3_free(sql);
        sqlite3_close(db);

        printf("%s[INFO]%s : printed %d row(s)%s\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, row, read_limit > 0 ? "" : " (whole table)");
    }
#pragma endregion SQLRead

    cllist_pool memory = {0}; /* must start zeroed: cllist_initMemory reads slabsd to tell init from grow */
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

    for (int i = 0; i < calculator_operators.size; ++i) {
        clist_destroy((clist *)clist_fast_at(&calculator_operators, i)->as.p);
    }
    clist_fast_destroy(&calculator_operators);

    clist_destroy(&calculator_display);

    /* cllist_freeMemory only hands a single node back to the pool, so it leaves
     * the slabs themselves alive. my_list has just the display in it, and the
     * destroy below reclaims the whole pool in one go. */
    cllist_destroyMemory(&memory);

    printf("%s[INFO]%s : %sSuccessfully freed memory%s.\n", hex_to_ansi(COLOR_INFOX, &arena, false).pointer, COLOR_RESET, "\033[35m", COLOR_RESET);

    return 0;
}

/* ------------------------------------------------------------------------------------------ */

#ifdef  __cplusplus
    }
#endif
