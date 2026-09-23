#ifdef __cplusplus
extern "C" {
#endif

#ifndef CEN__LIST_C_H
#define CEN__LIST_C_H

#include "../include/cbool.h" // IWYU pragma: keep
#include "../include/clist.h"
#include <stdio.h>

/* ==========================================================
 * clist implementation ([P1] [P2] ... -> [D1]) style
 * ========================================================== */

clist clist_init(void) {
    clist list;
    list.size = 0;
    list.capacity = 4;
    list.data = (clist_item_t **)malloc(list.capacity * sizeof(clist_item_t *));
    return list;
}

static void clist_ensure_capacity(clist *list) {
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->data = (clist_item_t **)realloc(list->data, list->capacity * sizeof(clist_item_t *));
    }
}

static clist_item_t *create_item(clist_type_t type) {
    clist_item_t *item = (clist_item_t *)malloc(sizeof(clist_item_t));
    item->type = type;
    return item;
}

clist *clist_append_int(clist *list, int val) {
    clist_ensure_capacity(list);
    clist_item_t *item = create_item(CLIST_TYPE_INT);
    item->as.i = val;
    list->data[list->size++] = item;
    return list;
}

clist *clist_append_float(clist *list, float val) {
    clist_ensure_capacity(list);
    clist_item_t *item = create_item(CLIST_TYPE_FLOAT);
    item->as.f = val;
    list->data[list->size++] = item;
    return list;
}

clist *clist_append_double(clist *list, double val) {
    clist_ensure_capacity(list);
    clist_item_t *item = create_item(CLIST_TYPE_DOUBLE);
    item->as.d = val;
    list->data[list->size++] = item;
    return list;
}

clist *clist_append_char(clist *list, char val) {
    clist_ensure_capacity(list);
    clist_item_t *item = create_item(CLIST_TYPE_CHAR);
    item->as.c = val;
    list->data[list->size++] = item;
    return list;
}

clist *clist_append_str(clist *list, const char *val) {
    clist_ensure_capacity(list);
    clist_item_t *item = create_item(CLIST_TYPE_STR);
    item->as.s = val ? strdup(val) : NULL;
    list->data[list->size++] = item;
    return list;
}

clist *clist_append_ptr(clist *list, void *val) {
    clist_ensure_capacity(list);
    clist_item_t *item = create_item(CLIST_TYPE_PTR);
    item->as.p = val;
    list->data[list->size++] = item;
    return list;
}

clist_item_t *clist_at(const clist *list, int index) {
    if (index < 0) index += list->size;
    if (index < 0 || index >= list->size) return NULL;
    return list->data[index];
}

clist_item_t **clist_next(clist_item_t **ptr) {
    return ptr ? ptr + 1 : NULL;
}

clist clist_slice(const clist *list, int start, int stop, int step) {
    clist sliced = clist_init();
    if (step == 0) return sliced;

    if (start < 0) start += list->size;
    if (stop < 0) stop += list->size;
    if (start < 0) start = 0;
    if (stop > list->size) stop = list->size;

    if (step > 0) {
        for (int i = start; i < stop; i += step) {
            if (i >= list->size) break;
            clist_item_t *src = list->data[i];
            clist_item_t *item = create_item(src->type);
            item->as = src->as;
            if (src->type == CLIST_TYPE_STR && src->as.s) {
                item->as.s = strdup(src->as.s);
            }
            clist_ensure_capacity(&sliced);
            sliced.data[sliced.size++] = item;
        }
    } else {
        for (int i = start; i > stop; i += step) {
            if (i < 0) break;
            clist_item_t *src = list->data[i];
            clist_item_t *item = create_item(src->type);
            item->as = src->as;
            if (src->type == CLIST_TYPE_STR && src->as.s) {
                item->as.s = strdup(src->as.s);
            }
            clist_ensure_capacity(&sliced);
            sliced.data[sliced.size++] = item;
        }
    }
    return sliced;
}

clist *clist_delete(clist *list, int index) {
    if (index < 0) index += list->size;
    if (index < 0 || index >= list->size) return list;

    if (list->data[index]->type == CLIST_TYPE_STR && list->data[index]->as.s) {
        free(list->data[index]->as.s);
    }
    free(list->data[index]);

    for (int i = index; i < list->size - 1; i++) {
        list->data[i] = list->data[i + 1];
    }
    list->size--;
    return list;
}

clist *clist_destroy(clist *list) {
    if (!list) return NULL;
    for (int i = 0; i < list->size; i++) {
        if (list->data[i]->type == CLIST_TYPE_STR && list->data[i]->as.s) {
            free(list->data[i]->as.s);
        }
        free(list->data[i]);
    }
    free(list->data);
    list->size = 0;
    list->capacity = 0;
    list->data = NULL;
    return list;
}

void clist_print(const clist *list, clist_print_mode_t mode) {
    if (!list) {
        printf("(null clist)\n");
        return;
    }

    if (mode == CLIST_PRINT_HEADING) {
        printf("clist { size = %d, capacity = %d, data_addr = %p }\n", list->size, list->capacity, (void *)list->data);
        return;
    }

    if (mode == CLIST_PRINT_ARRAY) {
        printf("[ ");
        for (int i = 0; i < list->size; i++) {
            clist_item_t *item = list->data[i];
            switch (item->type) {
                case CLIST_TYPE_INT: printf("%d", item->as.i); break;
                case CLIST_TYPE_FLOAT: printf("%ff", item->as.f); break;
                case CLIST_TYPE_DOUBLE: printf("%lf", item->as.d); break;
                case CLIST_TYPE_CHAR: printf("'%c'", item->as.c); break;
                case CLIST_TYPE_STR: printf("\"%s\"", item->as.s ? item->as.s : "nil"); break;
                case CLIST_TYPE_PTR: printf("%p", item->as.p); break;
            }
            if (i < list->size - 1) printf(", ");
        }
        printf(" ]\n");
        return;
    }

    if (mode == CLIST_PRINT_PRETTY) {
        printf("clist (pretty pointer indirection):\n");
        for (int i = 0; i < list->size; i++) {
            clist_item_t *item = list->data[i];
            printf("  [P%d @ %p] \033[1;32m->\033[0m [ ", i, (void *)item);
            switch (item->type) {
                case CLIST_TYPE_INT: printf("INT: %d", item->as.i); break;
                case CLIST_TYPE_FLOAT: printf("FLOAT: %f", item->as.f); break;
                case CLIST_TYPE_DOUBLE: printf("DOUBLE: %lf", item->as.d); break;
                case CLIST_TYPE_CHAR: printf("CHAR: '%c'", item->as.c); break;
                case CLIST_TYPE_STR: printf("STR: \"%s\"", item->as.s ? item->as.s : "nil"); break;
                case CLIST_TYPE_PTR: printf("PTR: %p", item->as.p); break;
            }
            printf(" ]\n");
        }
        return;
    }
}
_cmp_val clist_cmp__pointers_def(clist_item_t *self, clist_item_t *other) {
    #define MACRO_TO_STRING(x) #x

    if (strcmp(MACRO_TO_STRING(clist__cmp_pointers_impl), "") == 0) {
        fprintf(stderr, "[ERROR] : no definition for comparing pointers... aborting.\n");
        return false;
    }

    return true;
}
_cmp_val clist_cmp(const clist *self, const clist *other) {
    /* iterate through later, first compare the lengths */
    if (self->size != other->size)
        return self->size < other->size ? LESSER : GREATER;
    for (int i = 0; i < self->size; ++i) {
        if (self->data[i]->type == CLIST_TYPE_STR &&
            other->data[i]->type == CLIST_TYPE_STR)
            return strcmp(self->data[i]->as.s, other->data[i]->as.s);

        if (self->data[i]->type != CLIST_TYPE_PTR &&
            other->data[i]->type != CLIST_TYPE_PTR &&
            self->data[i]->type == other->data[i]->type
        ) {
            switch (self->data[i]->type) {
                case CLIST_TYPE_INT:    return (self->data[i]->as.i > other->data[i]->as.i) ?
                        GREATER : (self->data[i]->as.i == other->data[i]->as.i) ?
                         :
                        LESSER; break;
                case CLIST_TYPE_FLOAT:  return (self->data[i]->as.f > other->data[i]->as.f) ?
                        GREATER : (self->data[i]->as.f == other->data[i]->as.f) ?
                         :
                        LESSER; break;
                case CLIST_TYPE_DOUBLE: return (self->data[i]->as.d > other->data[i]->as.d) ?
                        GREATER : (self->data[i]->as.f == other->data[i]->as.f) ?
                         :
                        LESSER; break;
                case CLIST_TYPE_CHAR:   return (self->data[i]->as.c > other->data[i]->as.c) ?
                        GREATER : (self->data[i]->as.f == other->data[i]->as.f) ?
                         :
                        LESSER; break;

                case CLIST_TYPE_STR:    return strcmp(self->data[i]->as.s, other->data[i]->as.s); break;

                /* do nothing, successfully */
                default: ;
            }
        }

        else { clist_cmp__pointers_def(self->data[i]->as.p, other->data[i]->as.p); }

        /* go through indexes, compare if they match..
         * well for the numbers INT, FLOAT, DOUBLE that is easy alongside
         * the chars... but for the strings  & pointers leave that out
         * since list has ... */
    }
    return EQUAL;
}


/* ==========================================================
 * clist_fast implementation ([D1] [D2] ... [Dend])
 * =================================================         */

clist_fast clist_fast_init(void) {
    clist_fast list;
    list.size = 0;
    list.capacity = 4;
    list.data = (clist_item_t *)malloc(list.capacity * sizeof(clist_item_t));
    return list;
}

static void clist_fast_ensure_capacity(clist_fast *list) {
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->data = (clist_item_t *)realloc(list->data, list->capacity * sizeof(clist_item_t));
    }
}

clist_fast *clist_fast_append_int(clist_fast *list, int val) {
    clist_fast_ensure_capacity(list);
    list->data[list->size].type = CLIST_TYPE_INT;
    list->data[list->size].as.i = val;
    list->size++;
    return list;
}

clist_fast *clist_fast_append_float(clist_fast *list, float val) {
    clist_fast_ensure_capacity(list);
    list->data[list->size].type = CLIST_TYPE_FLOAT;
    list->data[list->size].as.f = val;
    list->size++;
    return list;
}

clist_fast *clist_fast_append_double(clist_fast *list, double val) {
    clist_fast_ensure_capacity(list);
    list->data[list->size].type = CLIST_TYPE_DOUBLE;
    list->data[list->size].as.d = val;
    list->size++;
    return list;
}

clist_fast *clist_fast_append_char(clist_fast *list, char val) {
    clist_fast_ensure_capacity(list);
    list->data[list->size].type = CLIST_TYPE_CHAR;
    list->data[list->size].as.c = val;
    list->size++;
    return list;
}

clist_fast *clist_fast_append_str(clist_fast *list, const char *val) {
    clist_fast_ensure_capacity(list);
    list->data[list->size].type = CLIST_TYPE_STR;
    list->data[list->size].as.s = val ? strdup(val) : NULL;
    list->size++;
    return list;
}

clist_fast *clist_fast_append_ptr(clist_fast *list, void *val) {
    clist_fast_ensure_capacity(list);
    list->data[list->size].type = CLIST_TYPE_PTR;
    list->data[list->size].as.p = val;
    list->size++;
    return list;
}

clist_item_t *clist_fast_at(const clist_fast *list, int index) {
    if (index < 0) index += list->size;
    if (index < 0 || index >= list->size) return NULL;
    return &list->data[index];
}

clist_item_t *clist_fast_next(clist_item_t *ptr) {
    return ptr ? ptr + 1 : NULL;
}

clist_fast clist_fast_slice(const clist_fast *list, int start, int stop, int step) {
    clist_fast sliced = clist_fast_init();
    if (step == 0) return sliced;

    if (start < 0) start += list->size;
    if (stop < 0) stop += list->size;
    if (start < 0) start = 0;
    if (stop > list->size) stop = list->size;

    if (step > 0) {
        for (int i = start; i < stop; i += step) {
            if (i >= list->size) break;
            clist_item_t *src = &list->data[i];
            clist_fast_ensure_capacity(&sliced);
            sliced.data[sliced.size].type = src->type;
            sliced.data[sliced.size].as = src->as;
            if (src->type == CLIST_TYPE_STR && src->as.s) {
                sliced.data[sliced.size].as.s = strdup(src->as.s);
            }
            sliced.size++;
        }
    } else {
        for (int i = start; i > stop; i += step) {
            if (i < 0) break;
            clist_item_t *src = &list->data[i];
            clist_fast_ensure_capacity(&sliced);
            sliced.data[sliced.size].type = src->type;
            sliced.data[sliced.size].as = src->as;
            if (src->type == CLIST_TYPE_STR && src->as.s) {
                sliced.data[sliced.size].as.s = strdup(src->as.s);
            }
            sliced.size++;
        }
    }
    return sliced;
}

clist_fast *clist_fast_delete(clist_fast *list, int index) {
    if (index < 0) index += list->size;
    if (index < 0 || index >= list->size) return list;

    if (list->data[index].type == CLIST_TYPE_STR && list->data[index].as.s) {
        free(list->data[index].as.s);
    }

    for (int i = index; i < list->size - 1; i++) {
        list->data[i] = list->data[i + 1];
    }
    list->size--;
    return list;
}

clist_fast *clist_fast_destroy(clist_fast *list) {
    if (!list) return NULL;
    for (int i = 0; i < list->size; i++) {
        if (list->data[i].type == CLIST_TYPE_STR && list->data[i].as.s) {
            free(list->data[i].as.s);
        }
    }
    free(list->data);
    list->size = 0;
    list->capacity = 0;
    list->data = NULL;
    return list;
}

void clist_fast_print(const clist_fast *list, clist_print_mode_t mode) {
    if (!list) {
        printf("(null clist_fast)\n");
        return;
    }

    if (mode == CLIST_PRINT_HEADING) {
        printf("clist_fast { size = %d, capacity = %d, data_addr = %p }\n", list->size, list->capacity, (void *)list->data);
        return;
    }

    if (mode == CLIST_PRINT_ARRAY) {
        printf("[ ");
        for (int i = 0; i < list->size; i++) {
            clist_item_t *item = &list->data[i];
            switch (item->type) {
                case CLIST_TYPE_INT: printf("%d", item->as.i); break;
                case CLIST_TYPE_FLOAT: printf("%ff", item->as.f); break;
                case CLIST_TYPE_DOUBLE: printf("%lf", item->as.d); break;
                case CLIST_TYPE_CHAR: printf("'%c'", item->as.c); break;
                case CLIST_TYPE_STR: printf("\"%s\"", item->as.s ? item->as.s : "nil"); break;
                case CLIST_TYPE_PTR: printf("%p", item->as.p); break;
            }
            if (i < list->size - 1) printf(", ");
        }
        printf(" ]\n");
        return;
    }

    if (mode == CLIST_PRINT_PRETTY) {
        printf("clist_fast (contiguous block):\n");
        for (int i = 0; i < list->size; i++) {
            clist_item_t *item = &list->data[i];
            printf("  [Item %d @ %p] \033[1;32m->\033[0m [ ", i, (void *)item);
            switch (item->type) {
                case CLIST_TYPE_INT: printf("INT: %d", item->as.i); break;
                case CLIST_TYPE_FLOAT: printf("FLOAT: %f", item->as.f); break;
                case CLIST_TYPE_DOUBLE: printf("DOUBLE: %lf", item->as.d); break;
                case CLIST_TYPE_CHAR: printf("CHAR: '%c'", item->as.c); break;
                case CLIST_TYPE_STR: printf("STR: \"%s\"", item->as.s ? item->as.s : "nil"); break;
                case CLIST_TYPE_PTR: printf("PTR: %p", item->as.p); break;
            }
            printf(" ]\n");
        }
        return;
    }
}

#endif

#ifdef __cplusplus
}
#endif
