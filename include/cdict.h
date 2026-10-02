#ifdef __cplusplus
    extern "C" {
#endif

/* There are no other variants, hence why we call it generic `dict` */
#ifndef CEN__DICT_H
#define CEN__DICT_H 1

#define CEN__DICT_VERSION 20261002L
#define CEN__DICT_VERSION__UNSTABLE 20261002L

#include "../include/cbool.h"
#include "../include/calias.h"
#include "../include/cstr.h"

#include <stdint.h>

/* --- tunables --- */
/* entries and buckets are always kept at a power of two so the index math is
 * a mask instead of a modulo. start here and grow by doubling from here. */
#define DICT_MIN_CAPACITY 16u

/* grow when (used + dead) reaches this fraction of the entry array. 7/10 keeps
 * the probe sequences short enough to stay inside a couple of cache lines while
 * leaving headroom so a burst of inserts doesn't rehash on every single one. */
#define DICT_LOAD_NUM 7u
#define DICT_LOAD_DEN 10u

/* buckets are 4 bytes per slot. sizing them at 1:1 with entries would waste a
 * quarter of the probe range; 1:2 is the usual compromise for open addressing.
 * kept only for reference: the first implementation probes `entries` directly
 * and never builds a bucket table. see the comment on `dict`. */
#define DICT_BUCKET_RATIO 2u

/* entry array stride. this is deliberately a power of two no larger than the
 * 64 byte cache line, so entries never straddle a line and two entries share
 * one. keep the struct's natural size equal to this. */
#define DICT_ENTRY_ALIGN 64u

/* header written just before every arena string payload. 8 rather than 4 so
 * the payload stays 8 byte aligned and DICT_STR_LENGTH can read a u32. */
#define DICT_STR_HEADER 8u

/* every arena allocation is rounded up to this, so the header of one string
 * never lands on a misaligned address behind the previous one. */
#define DICT_STR_ALIGN 8u

/* arena slab size. one malloc per slab, so this is a straight time/memory
 * trade: small slabs keep short keys dense, big slabs waste less on the slab
 * header and improve the hit rate. */
#define DICT_SLAB_SIZE 4096u

/* --- value tags --- */
/* why the dict needs its own tag rather than reusing clist_type_t: a dict has
 * to tell an EMPTY slot from a live zero, and it has to say which of the
 * multi-views of a void * (address / value / IEEE / string / RO memory) is
 * meant. the macros below move bits in and out; this says how to read them. */
typedef enum {
    DICT_TYPE_NONE = 0, /* slot is unused or a tombstone */
    DICT_TYPE_INT,
    DICT_TYPE_FLOAT,
    DICT_TYPE_DOUBLE,
    DICT_TYPE_CHAR,
    DICT_TYPE_STR, /* bytes are owned by the arena, never a borrowed pointer */
    DICT_TYPE_PTR
} dict_type_t;

typedef enum {
    DICT_STATE_EMPTY = 0, /* never used, terminates a probe run */
    DICT_STATE_USED,      /* live entry */
    DICT_STATE_DEAD       /* tombstone: a probe must continue past it */
} dict_state_t;

typedef enum {
    DICT_PRINT_ARRAY = 0, /* key -> val, one per line */
    DICT_PRINT_PRETTY,    /* with the slot state and tag spelled out */
    DICT_PRINT_HEADING    /* column names only */
} dict_print_mode_t;

/* --- the punning macros --- */
/* one void * slot is a union: the same 8 bytes read as an address, an integer,
 * an IEEE value or a byte address. packing is what makes it free to store a
 * key or a value of any of those shapes without a union or an allocation.
 * these only move the bits. hashing and comparing still need dict_type_t. */
#ifndef __cplusplus
    #define DICT_MEM_INT(i) \
        ((void*)(uintptr_t)((union { i32 i_val; uintptr_t ui_val; }){ .ui_val = 0, .i_val = (i) }).ui_val)
    #define DICT_MEM_FLOAT(f) \
        ((void*)(uintptr_t)((union { f32 f_val; uintptr_t ui_val; }){ .ui_val = 0, .f_val = (f) }).ui_val)
    #define DICT_MEM_DOUBLE(d) \
        ((void*)(uintptr_t)((union { f64 d_val; uintptr_t ui_val; }){ .ui_val = 0, .d_val = (d) }).ui_val)
    #define DICT_MEM_CHAR(c) \
        ((void*)(uintptr_t)((union { char c_val; uintptr_t ui_val; }){ .ui_val = 0, .c_val = (c) }).ui_val)
    #define DICT_MEM_PTR(p) ((void *)(p))

    /* --- unpack --- */
    #define DICT_INT(p)   \
        (((union { uintptr_t ui_val; i32 i_val; }){ .ui_val = (uintptr_t)(p) }).i_val)
    #define DICT_FLOAT(p) \
        (((union { uintptr_t ui_val; f32 f_val; }){ .ui_val = (uintptr_t)(p) }).f_val)
    #define DICT_DOUBLE(p) \
        (((union { uintptr_t ui_val; f64 f_val; }){ .ui_val = (uintptr_t)(p) }).f_val)
    #define DICT_CHAR(p)   \
        (((union { uintptr_t ui_val; char c_val; }){ .ui_val = (uintptr_t)(p) }).c_val)
    #define DICT_PTR(p)   ((void *)(p))
#else
union __dict_ptrconvert {
    void *p_val;
    uintptr_t ui_val;
    i32 i_val;
    f32 f32_val;
    f64 f64_val;
    char c_val;
};

#define DICT_MEM_INT(i)    (((union __dict_ptrconvert){.i_val = (i)}).p_val)
#define DICT_MEM_FLOAT(f)  (((union __dict_ptrconvert){.f32_val = (f)}).p_val)
#define DICT_MEM_DOUBLE(d) (((union __dict_ptrconvert){.f64_val = (d)}).p_val)
#define DICT_MEM_CHAR(c)   (((union __dict_ptrconvert){.c_val = (c)}).p_val)
#define DICT_MEM_PTR(p)    ((void *)(p))

/* unpack through uintptr_t rather than straight into p_val: these are called
 * with `const void *` keys, and C++ will not let a const void * initialize a
 * void * union member. going via the integer keeps the cast legal in C++ and
 * C99 alike. */
#define DICT_INT(p)   (((union __dict_ptrconvert){.ui_val = (uintptr_t)(p)}).i_val)
#define DICT_FLOAT(p) (((union __dict_ptrconvert){.ui_val = (uintptr_t)(p)}).f32_val)
#define DICT_DOUBLE(p)(((union __dict_ptrconvert){.ui_val = (uintptr_t)(p)}).f64_val)
#define DICT_CHAR(p)  (((union __dict_ptrconvert){.ui_val = (uintptr_t)(p)}).c_val)
#define DICT_PTR(p)   ((void *)(p))
#endif

/* --- storage classes ---
 * These are two different mechanisms on purpose. The arena is the only
 * free-list; using a pool for the entry array would fragment and could never
 * compact, since entries do not die one at a time. */

/* one arena slab. the header is what makes the chain walkable, so that
 * dict_arena_destroy can reach every slab without the payload needing a
 * back-pointer or the caller guessing at sizes. */
typedef struct dict_slab {
    struct dict_slab *next; /* chain, newest first */
    size_t size;            /* usable payload bytes that follow this header */
} dict_slab;

/* string bytes: bump allocated, reclaimed all at once, never individually.
 * the flip side is that a removed string's bytes stay put until the arena is
 * reset. that is the trade for never calling free on a string. */
typedef struct {
    dict_slab *slabs;   /* head of the chain; frees everything it reaches */
    dict_slab *current; /* the slab we are bumping into */
    char *buffer;       /* == (char *)current + sizeof(dict_slab) */
    size_t offset;      /* bump position inside buffer */
    size_t capacity;    /* usable bytes in buffer */
    size_t live;        /* bytes committed by live strings */
    u32 key_count;
} dict_arena;

/* one key/value slot. 32 bytes, a power of two, so a 64 byte aligned array of
 * these packs two per cache line and never straddles one. */
typedef struct {
    void *key;      /* punned, or arena bytes when key_type is STR */
    void *val;      /* punned, or arena bytes when val_type is STR */
    u64 hash;       /* cached, so rehash-on-grow is nearly free */
    u8 key_type;    /* dict_type_t: how to read the key slot */
    u8 val_type;    /* dict_type_t: how to read the val slot */
    u8 state;       /* dict_state_t */
    u8 pad;         /* explicit, so the struct size is exactly 32 */
} dict_entry_t;

/* the generic unordered map. empty means: call dict_init() before anything.
 *
 * there is deliberately no bucket array. slot selection is a linear probe over
 * `entries` starting at hash & mask, so a bucket table would only recompute the
 * slot a second time. buckets earn their keep when probing by chaining; here
 * that would be a second structure describing the same thing. */
typedef struct {
    dict_entry_t *entries; /* dense, power-of-two length, 64 byte aligned */

    u32 capacity; /* length of entries, always a power of two */
    u32 mask;     /* capacity - 1, so slot = hash & mask */
    u32 used;     /* live entries, this is what dict_len reports */
    u32 dead;     /* tombstones, drives compaction rather than growth */

    dict_arena *strings; /* owns every DICT_TYPE_STR key and val */
    bool owns_strings;   /* false when strings points at a caller arena */
} dict;

/* --- API --- */
/* there is no "init this struct I already have" entry point on purpose. it
 * would leave dict_destroy unable to tell whether the dict struct itself is
 * heap memory it owns, which is one crash waiting to happen. dict_initArena
 * still lets you lend it your arena. */
dict *dict_init(void);
dict *dict_initCapacity(u32 capacity);
dict *dict_initArena(dict_arena *arena); /* borrows the caller's arena */
void  dict_destroy(dict *self);

/* keys go in already punned (DICT_MEM_*), values are passed by their real type
 * so the setter does the punning and can tag the slot at the same time. */
dict *dict_set_int(dict *self, void *key, i32 val);
dict *dict_set_float(dict *self, void *key, f32 val);
dict *dict_set_double(dict *self, void *key, f64 val);
dict *dict_set_char(dict *self, void *key, char val);
dict *dict_set_str(dict *self, const char *key, const char *val); /* both are copied */
dict *dict_set_ptr(dict *self, void *key, void *val);

/* --- CEN string variants ---
 * str, sstr and tstr are all just "bytes plus a length", so the dict stores
 * all three the same way: the bytes are copied into the arena and the length is
 * written in a small header just before them. one tag, DICT_TYPE_STR, covers
 * all of them, because once copied there is nothing left to tell them apart.
 *
 * note that sstr is not a small string: sstr.pointer is char[65535] because
 * CEN__SSTR_MAX_LEN is 65535. only tstr is genuinely bounded, at 255 bytes by
 * its u8 length. passing an sstr by value therefore copies 64 KiB per call. */
/* str and tstr are 16 bytes each, so by value is fine. sstr is char[65535],
 * so it goes by pointer in both directions: by value would ask the caller for a
 * 128 KiB stack frame just to pass a key and a value. */
dict *dict_set_string(dict *self, str key, str val);
dict *dict_set_strings(dict *self, const sstr *key, const sstr *val);
dict *dict_set_stringt(dict *self, tstr key, tstr val);

dict *dict_setTyped(dict *self, void *key, dict_type_t key_type, void *val, dict_type_t val_type);

bool  dict_get_int(dict *self, void *key, i32 *out);
bool  dict_get_float(dict *self, void *key, f32 *out);
bool  dict_get_double(dict *self, void *key, f64 *out);
bool  dict_get_char(dict *self, void *key, char *out);
bool  dict_get_str(dict *self, const char *key, const char **out); /* borrowed, arena owned */
bool  dict_get_ptr(dict *self, void *key, void **out);

bool  dict_get_string(dict *self, str key, str *out);           /* borrowed, arena owned */
bool  dict_get_strings(dict *self, const sstr *key, sstr *out); /* copies out, see note on set_strings */
bool  dict_get_stringt(dict *self, tstr key, tstr *out);         /* borrowed, arena owned */

dict_entry_t *dict_get(dict *self, void *key, dict_type_t key_type);
dict_entry_t *dict_get_strEntry(dict *self, const char *key);
dict_entry_t *dict_get_stringEntry(dict *self, str key);
dict_entry_t *dict_at(dict *self, u32 index, dict_state_t state); /* live entries only */

bool  dict_has(dict *self, void *key, dict_type_t key_type);
bool  dict_has_str(dict *self, const char *key);
bool  dict_has_string(dict *self, str key);
bool  dict_remove(dict *self, void *key, dict_type_t key_type);
bool  dict_remove_str(dict *self, const char *key);
bool  dict_remove_string(dict *self, str key);
bool  dict_clear(dict *self); /* empties the table, the arena survives */

u32   dict_len(const dict *self);
u32   dict_capacity(const dict *self);
/* length is ignored for every type except DICT_TYPE_STR, where it is the whole
 * point: without it a string key would hash its own address, not its contents. */
u64   dict_hash(const void *key, size_t length, dict_type_t key_type);
void  dict_print(const dict *self, dict_print_mode_t mode);

void  dict_arena_init(dict_arena *self);
void  dict_arena_destroy(dict_arena *self);
char *dict_arena_alloc(dict_arena *self, size_t bytes);
char *dict_arena_copyString(dict_arena *self, const char *bytes, size_t length);
void  dict_arena_reset(dict_arena *self);

/* reads back the length that dict_arena_copyString wrote. the payload lives at
 * p, so the header sits DICT_STR_HEADER bytes behind it. */
#define DICT_STR_LENGTH(p) (*(const u32 *)((const char *)(p) - DICT_STR_HEADER))

#endif

#ifdef __cplusplus
    }
#endif
