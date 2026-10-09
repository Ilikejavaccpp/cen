#ifdef __cplusplus
extern "C" {
#endif

#ifndef CEN__DICT_C_H
#define CEN__DICT_C_H

#include "../include/calias.h"
#include "../include/cdict.h"
#include "../include/cdict__strk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* aligned_alloc is C11 and needs the size to be a multiple of the alignment,
 * which we already arrange. older toolchains get the malloc over-align trick. */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
static void *dict_alignedAlloc(size_t alignment, size_t bytes) {
    /* one word is reserved for the pointer we must keep in order to free this.
     * the alignment is computed starting *after* that word, never from base
     * itself: if base is already aligned then aligning from base would return
     * base, and stashing the free pointer at aligned - 8 would write one word
     * in front of the allocation. */
    size_t total = bytes + alignment + sizeof(void *);
    char *base = (char *)malloc(total);
    if (base == NULL)
        return NULL;

    uintptr_t start = (uintptr_t)(base + sizeof(void *));
    uintptr_t aligned = (start + alignment - 1) & ~(uintptr_t)(alignment - 1);

    ((void **)aligned)[-1] = base; /* sits inside the block, at or above base */
    return (void *)aligned;
}

static void dict_alignedFree(void *ptr) {
    if (ptr == NULL)
        return;
    free(((void **)ptr)[-1]);
}
#define dict__aligned_alloc(alignment, bytes) dict_alignedAlloc((alignment), (bytes))
#define dict__aligned_free(ptr)              dict_alignedFree(ptr)
#else
#define dict__aligned_alloc(alignment, bytes) aligned_alloc((alignment), (bytes))
#define dict__aligned_free(ptr)              free(ptr)
#endif

/* =====================================================================
 * how a lookup actually works, in the order you need to read this file:
 *
 *   1. hash   the key into a u64                      (dict_hash)
 *   2. slot   hash & mask lands on a start position   (mask == capacity - 1)
 *   3. probe  walk forward, +1 & mask, wrapping       (dict_findSlot)
 *
 * an EMPTY slot ends the walk, because nothing can ever be past it. a DEAD
 * slot is stepped over, which is the whole reason DEAD exists: a removal
 * must not cut a probe run in half, or every key sitting behind the hole
 * becomes unreachable. DEAD slots are reclaimed by rebuilding the table once
 * they outnumber the live ones, so the walk stays short.
 * ===================================================================== */

/* ======================== the arena ======================== */

/* bump allocator for string bytes. one malloc per slab, no free per string:
 * a string lives until the whole arena is reset or destroyed. the trade is
 * deliberate, since a per-string free would defeat the point of the slab. */

void dict_arena_init(dict_arena *self) {
    if (self == NULL)
        return;
    /* all-zero is already a valid empty arena, so this is just spelling that out */
    memset(self, 0, sizeof(dict_arena));
}

static dict_slab *dict_arena_newSlab(size_t bytes) {
    size_t total = sizeof(dict_slab) + bytes;
    /* round the whole block up to the cache line so the payload behind the
     * header stays aligned for whatever ends up stored there */
    total = (total + (DICT_ENTRY_ALIGN - 1)) & ~((size_t)(DICT_ENTRY_ALIGN - 1));

    dict_slab *slab = (dict_slab *)dict__aligned_alloc(DICT_ENTRY_ALIGN, total);
    if (slab == NULL)
        return NULL;
    slab->next = NULL;
    slab->size = total - sizeof(dict_slab);
    return slab;
}

char *dict_arena_alloc(dict_arena *self, size_t bytes) {
    if (self == NULL || bytes == 0)
        return NULL;

    /* round the bump pointer up before handing anything out. without this,
     * consecutive strings pack back to back and a DICT_STR_HEADER u32 lands on
     * an odd address, which UBSan flags as a misaligned load. the slack is the
     * price of storing a length behind a bare void *. */
    self->offset = (self->offset + (DICT_STR_ALIGN - 1)) & ~((size_t)(DICT_STR_ALIGN - 1));

    if (self->current == NULL || self->offset + bytes > self->capacity) {
        /* a string bigger than a whole slab gets a slab of its own */
        size_t want = bytes > (size_t)DICT_SLAB_SIZE ? bytes : (size_t)DICT_SLAB_SIZE;
        dict_slab *slab = dict_arena_newSlab(want);
        if (slab == NULL)
            return NULL;

        slab->next = self->slabs; /* newest first, so the tail is the oldest */
        self->slabs = slab;
        self->current = slab;
        self->buffer = (char *)slab + sizeof(dict_slab);
        self->capacity = slab->size;
        self->offset = 0;
    }

    char *out = self->buffer + self->offset;
    self->offset += bytes;
    self->live += bytes;
    return out;
}

char *dict_arena_copyString(dict_arena *self, const char *bytes, size_t length) {
    if (self == NULL || bytes == NULL)
        return NULL;

    char *slot = dict_arena_alloc(self, DICT_STR_HEADER + length + 1);
    if (slot == NULL)
        return NULL;

    /* the length sits behind the payload, which is how a bare void * can still
     * answer "how long are you?" without widening dict_entry_t past 32 bytes */
    *(u32 *)slot = (u32)length;
    memcpy(slot + DICT_STR_HEADER, bytes, length);
    slot[DICT_STR_HEADER + length] = '\0'; /* NUL too, so it reads as a C string */

    return slot + DICT_STR_HEADER;
}

void dict_arena_reset(dict_arena *self) {
    if (self == NULL)
        return;
    /* slabs are kept so the next round of keys reuses the same memory. only
     * the bump pointer rewinds. free every slab with dict_arena_destroy. */
    self->current = NULL;
    self->buffer = NULL;
    self->offset = 0;
    self->capacity = 0;
    self->live = 0;
    self->key_count = 0;
}

void dict_arena_destroy(dict_arena *self) {
    if (self == NULL)
        return;

    dict_slab *slab = self->slabs;
    while (slab != NULL) {
        dict_slab *next = slab->next;
        dict__aligned_free(slab);
        slab = next;
    }
    dict_arena_init(self);
}

/* ======================== hashing ======================== */

/* splitmix64 finalizer. any hash can be salted and pushed through this to get
 * a well mixed u64, which matters because the low bits pick the slot and we
 * then throw away all but the low bits. */
static u64 dict_mix64(u64 x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

/* FNV-1a: a byte at a time, so equal strings hash equal wherever they live */
static u64 dict_hashBytes(const char *bytes, size_t length) {
    u64 hash = 0xcbf29ce484222325ULL; /* FNV offset basis */
    for (size_t i = 0; i < length; i++) {
        hash ^= (u64)(unsigned char)bytes[i];
        hash *= 0x100000001b3ULL; /* FNV prime */
    }
    return dict_mix64(hash);
}

u64 dict_hash(const void *key, size_t length, dict_type_t key_type) {
    switch (key_type) {
        case DICT_TYPE_INT:
            return dict_mix64((u64)(u32)DICT_INT(key));
        case DICT_TYPE_CHAR:
            return dict_mix64((u64)(unsigned char)DICT_CHAR(key));
        case DICT_TYPE_FLOAT: {
            union { f32 f; u32 u; } v;
            v.f = DICT_FLOAT(key);
            return dict_mix64((u64)v.u);
        }
        case DICT_TYPE_DOUBLE: {
            union { f64 d; u64 u; } v;
            v.d = DICT_DOUBLE(key);
            return dict_mix64(v.u);
        }
        case DICT_TYPE_STR:
            /* the reason string keys need their length: hashing the pointer
             * would make two equal strings in different slabs unequal keys */
            return dict_hashBytes((const char *)key, length);
        case DICT_TYPE_PTR:
        case DICT_TYPE_NONE:
        default:
            return dict_mix64((u64)(uintptr_t)key);
    }
}

static bool dict_keyEq(const dict_entry_t *entry, const void *key, size_t length, dict_type_t key_type) {
    if (entry->state != DICT_STATE_USED)
        return false;
    if (entry->key_type != (u8)key_type)
        return false;

    if (key_type == DICT_TYPE_STR)
        return memcmp(entry->key, key, length) == 0;

    /* every other key is a punned scalar, so comparing the 8 raw bytes of the
     * slot compares the value. note the float consequence: this is bit
     * equality, so +0.0 and -0.0 are different keys and a NaN equals itself. */
    return entry->key == key;
}

/* ======================== the table ======================== */

static bool dict_resize(dict *self, u32 capacity) {
    /* round up to a power of two, never below the floor. a power of two turns
     * the wrap into a mask, which is one AND instead of a division. */
    u32 target = DICT_MIN_CAPACITY;
    while (target < capacity)
        target <<= 1;

    size_t bytes = (size_t)target * sizeof(dict_entry_t);
    dict_entry_t *fresh = (dict_entry_t *)dict__aligned_alloc(DICT_ENTRY_ALIGN, bytes);
    if (fresh == NULL)
        return false;

    /* zeroed memory is exactly DICT_STATE_EMPTY + DICT_TYPE_NONE */
    memset(fresh, 0, bytes);

    dict_entry_t *old_entries = self->entries;
    u32 old_capacity = self->capacity;

    self->entries = fresh;
    self->capacity = target;
    self->mask = target - 1;
    self->used = 0;
    self->dead = 0;

    for (u32 i = 0; i < old_capacity; i++) {
        dict_entry_t *entry = &old_entries[i];
        if (entry->state != DICT_STATE_USED)
            continue;

        /* hashes were cached per entry, so growing never re-reads a string */
        u32 slot = (u32)(entry->hash & (u64)self->mask);
        while (self->entries[slot].state == DICT_STATE_USED)
            slot = (slot + 1) & self->mask;

        self->entries[slot] = *entry;
        self->used++;
    }

    dict__aligned_free(old_entries);
    return true;
}

static bool dict_growIfNeeded(dict *self) {
    u32 filled = self->used + self->dead;
    if ((u64)filled * (u64)DICT_LOAD_DEN < (u64)self->capacity * (u64)DICT_LOAD_NUM)
        return true; /* still under DICT_LOAD_NUM/DICT_LOAD_DEN */
    return dict_resize(self, self->capacity * 2);
}

static bool dict_compactIfNeeded(dict *self) {
    if (self->dead <= self->capacity / 2)
        return true;
    return dict_resize(self, self->capacity); /* same size, no tombstones */
}

static u32 dict_findSlot(dict *self, u64 hash, const void *key, size_t length, dict_type_t key_type, bool *found) {
    u32 index = (u32)(hash & (u64)self->mask);
    u32 reusable = 0;
    bool have_reusable = false;

    for (u32 step = 0; step < self->capacity; step++) {
        dict_entry_t *entry = &self->entries[index];

        if (entry->state == DICT_STATE_EMPTY) {
            /* nothing lives past an empty slot, so the key is absent. a
             * tombstone seen earlier is still a legal place to insert. */
            *found = false;
            return have_reusable ? reusable : index;
        }

        if (entry->state == DICT_STATE_DEAD) {
            if (!have_reusable) {
                reusable = index;
                have_reusable = true;
            }
        } else if (dict_keyEq(entry, key, length, key_type)) {
            *found = true;
            return index;
        }

        index = (index + 1) & self->mask;
    }

    /* every slot is DEAD or USED. a tombstone is the only space left. */
    *found = false;
    return have_reusable ? reusable : index;
}

static dict_entry_t *dict_lookup(dict *self, const void *key, size_t length, dict_type_t key_type) {
    if (self == NULL || self->entries == NULL || self->used == 0)
        return NULL;

    u64 hash = dict_hash(key, length, key_type);
    bool found = false;
    u32 index = dict_findSlot(self, hash, key, length, key_type, &found);
    if (!found)
        return NULL;
    return &self->entries[index];
}

static dict_entry_t *dict_claimSlot(dict *self, const void *key, size_t length, dict_type_t key_type, bool *inserted) {
    if (inserted != NULL)
        *inserted = false;
    if (self == NULL)
        return NULL;

    /* grow first: it can move every entry, so no slot may be held across it */
    if (!dict_growIfNeeded(self))
        return NULL;

    u64 hash = dict_hash(key, length, key_type);
    bool found = false;
    u32 index = dict_findSlot(self, hash, key, length, key_type, &found);
    dict_entry_t *entry = &self->entries[index];

    if (found)
        return entry; /* caller overwrites val in place, used is unchanged */

    if (entry->state == DICT_STATE_DEAD)
        self->dead--; /* a tombstone being reused stops being one */

    entry->state = DICT_STATE_USED;
    entry->hash = hash;
    entry->key = (void *)(uintptr_t)key;
    entry->key_type = (u8)key_type;
    entry->val = NULL;
    entry->val_type = DICT_TYPE_NONE;
    self->used++;

    if (inserted != NULL)
        *inserted = true;
    return entry;
}

/* ======================== lifecycle ======================== */

dict *dict_initCapacity(u32 capacity) {
    dict *self = (dict *)malloc(sizeof(dict));
    if (self == NULL)
        return NULL;
    memset(self, 0, sizeof(dict));

    self->strings = (dict_arena *)malloc(sizeof(dict_arena));
    if (self->strings == NULL) {
        free(self);
        return NULL;
    }
    dict_arena_init(self->strings);
    self->owns_strings = true;

    if (!dict_resize(self, capacity)) {
        free(self->strings);
        free(self);
        return NULL;
    }
    return self;
}

dict *dict_init(void) {
    return dict_initCapacity(DICT_MIN_CAPACITY);
}

dict *dict_initArena(dict_arena *arena) {
    dict *self = dict_initCapacity(DICT_MIN_CAPACITY);
    if (self == NULL)
        return NULL;

    if (self->strings != NULL) {
        dict_arena_destroy(self->strings);
        free(self->strings);
    }
    self->strings = arena;
    self->owns_strings = false; /* the arena is the caller's to destroy */
    return self;
}

void dict_destroy(dict *self) {
    if (self == NULL)
        return;

    dict__aligned_free(self->entries);
    if (self->owns_strings && self->strings != NULL) {
        dict_arena_destroy(self->strings);
        free(self->strings);
    }
    self->entries = NULL;
    free(self);
}

/* ======================== setters ======================== */

/* keys arrive already punned by DICT_MEM_*, values arrive as their real type
 * so the punning and the tag happen together and can never disagree. */
dict *dict_setTyped(dict *self, void *key, dict_type_t key_type, void *val, dict_type_t val_type) {
    /* no string keys here: they need a length to hash, so they go through
     * dict_set_str / dict_set_string* which copy into the arena first. */
    dict_entry_t *entry = dict_claimSlot(self, key, 0, key_type, NULL);
    if (entry == NULL)
        return NULL;
    entry->val = val;
    entry->val_type = (u8)val_type;
    return self;
}

dict *dict_set_int(dict *self, void *key, i32 val) {
    dict_entry_t *entry = dict_claimSlot(self, key, 0, DICT_TYPE_INT, NULL);
    if (entry == NULL)
        return NULL;
    entry->val = DICT_MEM_INT(val);
    entry->val_type = DICT_TYPE_INT;
    return self;
}

dict *dict_set_float(dict *self, void *key, f32 val) {
    dict_entry_t *entry = dict_claimSlot(self, key, 0, DICT_TYPE_FLOAT, NULL);
    if (entry == NULL)
        return NULL;
    entry->val = DICT_MEM_FLOAT(val);
    entry->val_type = DICT_TYPE_FLOAT;
    return self;
}

dict *dict_set_double(dict *self, void *key, f64 val) {
    dict_entry_t *entry = dict_claimSlot(self, key, 0, DICT_TYPE_DOUBLE, NULL);
    if (entry == NULL)
        return NULL;
    entry->val = DICT_MEM_DOUBLE(val);
    entry->val_type = DICT_TYPE_DOUBLE;
    return self;
}

dict *dict_set_char(dict *self, void *key, char val) {
    dict_entry_t *entry = dict_claimSlot(self, key, 0, DICT_TYPE_CHAR, NULL);
    if (entry == NULL)
        return NULL;
    entry->val = DICT_MEM_CHAR(val);
    entry->val_type = DICT_TYPE_CHAR;
    return self;
}

dict *dict_set_ptr(dict *self, void *key, void *val) {
    dict_entry_t *entry = dict_claimSlot(self, key, 0, DICT_TYPE_PTR, NULL);
    if (entry == NULL)
        return NULL;
    entry->val = val; /* stored as-is: the dict never owns a bare pointer */
    entry->val_type = DICT_TYPE_PTR;
    return self;
}

/* one path for every string flavour, because str / sstr / tstr all reduce to
 * "bytes plus a length" before they reach the table. */
static dict *dict_set_bytes(dict *self, const char *key, size_t key_length, const char *val, size_t val_length) {
    if (self == NULL || self->strings == NULL || key == NULL || val == NULL)
        return NULL;

    char *key_copy = dict_arena_copyString(self->strings, key, key_length);
    if (key_copy == NULL)
        return NULL;
    char *val_copy = dict_arena_copyString(self->strings, val, val_length);
    if (val_copy == NULL)
        return NULL;

    bool inserted = false;
    dict_entry_t *entry = dict_claimSlot(self, key_copy, key_length, DICT_TYPE_STR, &inserted);
    if (entry == NULL)
        return NULL;

    entry->val = val_copy;
    entry->val_type = DICT_TYPE_STR;
    if (inserted)
        self->strings->key_count++;

    /* overwriting an existing key leaves its old key and val bytes stranded in
     * the arena until the next reset. that is the bump allocator's price and
     * the reason repeated overwrite of long strings is worth watching. */
    return self;
}

dict *dict_set_str(dict *self, const char *key, const char *val) {
    if (key == NULL || val == NULL)
        return NULL;
    return dict_set_bytes(self, key, strlen(key), val, strlen(val));
}

dict *dict_set_string(dict *self, str key, str val) {
    if (key.pointer == NULL || val.pointer == NULL)
        return NULL;
    return dict_set_bytes(self, key.pointer, (size_t)key.length, val.pointer, (size_t)val.length);
}

/* taken by pointer, not by value: sstr is char[65535] + u16, so passing two of
 * them by value asks for a 128 KiB stack frame. str and tstr are 16 bytes each
 * and are safe to pass by value. */
dict *dict_set_strings(dict *self, const sstr *key, const sstr *val) {
    if (key == NULL || val == NULL)
        return NULL;
    size_t key_length = (size_t)key->length;
    size_t val_length = (size_t)val->length;
    if (key_length > CEN__SSTR_MAX_LEN)
        key_length = CEN__SSTR_MAX_LEN;
    if (val_length > CEN__SSTR_MAX_LEN)
        val_length = CEN__SSTR_MAX_LEN;
    return dict_set_bytes(self, key->pointer, key_length, val->pointer, val_length);
}

dict *dict_set_stringt(dict *self, tstr key, tstr val) {
    if (key.pointer == NULL || val.pointer == NULL)
        return NULL;
    return dict_set_bytes(self, key.pointer, (size_t)key.length, val.pointer, (size_t)val.length);
}

dict * dict_setPair(dict *self, dict_key_t key, void *val, dict_type_t vtype) {
    /* 1. Check if integer mask -- this is easy, not the core buggy parts */
    if (key.as == 1) // 1- TYPE_INT
    {
        dict_entry_t *entry = dict_claimSlot(self,
            key.integer,
            0,
            DICT_TYPE_INT, /* 1-- int, 0-- string */
            NULL);
        if (entry == NULL) return NULL;
        entry->val = val;
        entry->val_type = (u8)vtype;
    }

    /* 2. Check if string mask -- this is the hard part and where I either
     * fricked up a mem address or got a segfault */
    if (key.as == 0) {
        if (self == NULL || self->strings == NULL || key.pointer == NULL)
            return NULL;

        /* keep a copy of the string ,
         * only cost here is std string.h's `strlen` */
        char * key_cp = dict_arena_copyString(self->strings, key.pointer, strlen(key.pointer));
        if (key_cp == NULL) return NULL;

        bool inserted = false;
        dict_entry_t *entry = dict_claimSlot(self, key_cp, strlen(key.pointer), DICT_TYPE_STR, &inserted);
        if (entry != NULL && inserted)
            self->strings->key_count++;
        entry->val = val;
        entry->val_type = (u8)vtype;
    }

    return self;
}

/* ======================== getters ======================== */

static bool dict_get_bytes(dict *self, const char *key, size_t key_length, const char **out, size_t *out_length) {
    dict_entry_t *entry = dict_lookup(self, key, key_length, DICT_TYPE_STR);
    if (entry == NULL || entry->val_type != DICT_TYPE_STR)
        return false;

    if (out != NULL)
        *out = (const char *)entry->val;
    if (out_length != NULL)
        *out_length = (size_t)DICT_STR_LENGTH(entry->val);
    return true;
}

bool dict_get_int(dict *self, void *key, i32 *out) {
    dict_entry_t *entry = dict_lookup(self, key, 0, DICT_TYPE_INT);
    if (entry == NULL || entry->val_type != DICT_TYPE_INT)
        return false;
    if (out != NULL)
        *out = DICT_INT(entry->val);
    return true;
}

bool dict_get_float(dict *self, void *key, f32 *out) {
    dict_entry_t *entry = dict_lookup(self, key, 0, DICT_TYPE_FLOAT);
    if (entry == NULL || entry->val_type != DICT_TYPE_FLOAT)
        return false;
    if (out != NULL)
        *out = DICT_FLOAT(entry->val);
    return true;
}

bool dict_get_double(dict *self, void *key, f64 *out) {
    dict_entry_t *entry = dict_lookup(self, key, 0, DICT_TYPE_DOUBLE);
    if (entry == NULL || entry->val_type != DICT_TYPE_DOUBLE)
        return false;
    if (out != NULL)
        *out = DICT_DOUBLE(entry->val);
    return true;
}

bool dict_get_char(dict *self, void *key, char *out) {
    dict_entry_t *entry = dict_lookup(self, key, 0, DICT_TYPE_CHAR);
    if (entry == NULL || entry->val_type != DICT_TYPE_CHAR)
        return false;
    if (out != NULL)
        *out = DICT_CHAR(entry->val);
    return true;
}

bool dict_get_ptr(dict *self, void *key, void **out) {
    dict_entry_t *entry = dict_lookup(self, key, 0, DICT_TYPE_PTR);
    if (entry == NULL || entry->val_type != DICT_TYPE_PTR)
        return false;
    if (out != NULL)
        *out = entry->val;
    return true;
}

/* out == NULL is legal everywhere and means "just tell me it is there" */
bool dict_get_str(dict *self, const char *key, const char **out) {
    if (key == NULL)
        return false;
    return dict_get_bytes(self, key, strlen(key), out, NULL);
}

bool dict_get_string(dict *self, str key, str *out) {
    if (key.pointer == NULL)
        return false;

    const char *bytes = NULL;
    size_t length = 0;
    if (!dict_get_bytes(self, key.pointer, (size_t)key.length, &bytes, &length))
        return false;

    if (out != NULL) {
        out->pointer = (char *)bytes; /* borrowed: owned by the dict's arena */
        out->length = (int)length;
    }
    return true;
}

/* sstr owns its bytes inline, so there is nowhere to hand back a borrowed
 * pointer: the result has to be copied out. that is a 64 KiB worst case. */
bool dict_get_strings(dict *self, const sstr *key, sstr *out) {
    if (key == NULL)
        return false;

    const char *bytes = NULL;
    size_t length = 0;
    if (!dict_get_bytes(self, key->pointer, (size_t)key->length, &bytes, &length))
        return false;

    if (out != NULL) {
        if (length > CEN__SSTR_MAX_LEN)
            length = CEN__SSTR_MAX_LEN;
        memcpy(out->pointer, bytes, length);
        out->pointer[length] = '\0';
        out->length = (u16)length;
    }
    return true;
}

bool dict_get_stringt(dict *self, tstr key, tstr *out) {
    if (key.pointer == NULL)
        return false;

    const char *bytes = NULL;
    size_t length = 0;
    if (!dict_get_bytes(self, key.pointer, (size_t)key.length, &bytes, &length))
        return false;

    if (out != NULL) {
        out->pointer = (char *)bytes; /* borrowed: owned by the dict's arena */
        out->length = (u8)length;
    }
    return true;
}

/* ======================== raw entry access ======================== */

dict_entry_t *dict_get(dict *self, void *key, dict_type_t key_type) {
    return dict_lookup(self, key, 0, key_type);
}

dict_entry_t *dict_get_strEntry(dict *self, const char *key) {
    if (key == NULL)
        return NULL;
    return dict_lookup(self, key, strlen(key), DICT_TYPE_STR);
}

dict_entry_t *dict_get_stringEntry(dict *self, str key) {
    if (key.pointer == NULL)
        return NULL;
    return dict_lookup(self, key.pointer, (size_t)key.length, DICT_TYPE_STR);
}

/* index is a raw slot, not the nth live key. DICT_STATE_USED skips the empty
 * and dead slots; pass DICT_STATE_EMPTY to see the holes. */
dict_entry_t *dict_at(dict *self, u32 index, dict_state_t state) {
    if (self == NULL || self->entries == NULL || index >= self->capacity)
        return NULL;

    dict_entry_t *entry = &self->entries[index];
    if (state == DICT_STATE_USED && entry->state != DICT_STATE_USED)
        return NULL;
    return entry;
}

/* ======================== removal ======================== */

static bool dict_remove_entry(dict_entry_t *entry, dict *self) {
    if (entry == NULL)
        return false;

    u8 removed_key_type = entry->key_type;
    entry->state = DICT_STATE_DEAD; /* leave a tombstone, never a hole */
    entry->key = NULL;
    entry->val = NULL;
    entry->hash = 0;
    self->used--;
    self->dead++;

    if (removed_key_type == DICT_TYPE_STR && self->strings != NULL && self->strings->key_count > 0)
        self->strings->key_count--;

    /* the string bytes themselves stay put, the arena reclaims them wholesale */
    dict_compactIfNeeded(self);
    return true;
}

/* existence checks, declared in cdict.h. a lookup is enough: the value is
 * never touched, so this is read-only even though lookup is not const. */
bool dict_has(dict *self, void *key, dict_type_t key_type) {
    return dict_lookup(self, key, 0, key_type) != NULL;
}

bool dict_has_str(dict *self, const char *key) {
    if (key == NULL)
        return false;
    return dict_lookup(self, key, strlen(key), DICT_TYPE_STR) != NULL;
}

bool dict_has_string(dict *self, str key) {
    if (key.pointer == NULL)
        return false;
    return dict_lookup(self, key.pointer, (size_t)key.length, DICT_TYPE_STR) != NULL;
}

bool dict_remove(dict *self, void *key, dict_type_t key_type) {
    return dict_remove_entry(dict_lookup(self, key, 0, key_type), self);
}

/* string removal has to hash with the real length, so it cannot reuse
 * dict_remove's punned-key path */
static bool dict_remove_bytes(dict *self, const char *key, size_t key_length) {
    if (self == NULL || key == NULL)
        return false;
    return dict_remove_entry(dict_lookup(self, key, key_length, DICT_TYPE_STR), self);
}

bool dict_remove_str(dict *self, const char *key) {
    if (key == NULL)
        return false;
    return dict_remove_bytes(self, key, strlen(key));
}

bool dict_remove_string(dict *self, str key) {
    if (key.pointer == NULL)
        return false;
    return dict_remove_bytes(self, key.pointer, (size_t)key.length);
}

bool dict_clear(dict *self) {
    if (self == NULL || self->entries == NULL)
        return false;

    memset(self->entries, 0, (size_t)self->capacity * sizeof(dict_entry_t));
    self->used = 0;
    self->dead = 0;

    if (self->owns_strings && self->strings != NULL)
        dict_arena_reset(self->strings); /* a table clear should not strand the bytes */
    return true;
}

u32 dict_len(const dict *self) {
    return self == NULL ? 0 : self->used;
}

u32 dict_capacity(const dict *self) {
    return self == NULL ? 0 : self->capacity;
}

/* ======================== printing ======================== */

static const char *dict_typeName(u8 type) {
    switch (type) {
        case DICT_TYPE_INT: return "int";
        case DICT_TYPE_FLOAT: return "float";
        case DICT_TYPE_DOUBLE: return "double";
        case DICT_TYPE_CHAR: return "char";
        case DICT_TYPE_STR: return "str";
        case DICT_TYPE_PTR: return "ptr";
        case DICT_TYPE_NONE:
        default: return "none";
    }
}

static const char *dict_stateName(u8 state) {
    switch (state) {
        case DICT_STATE_USED: return "used";
        case DICT_STATE_DEAD: return "dead";
        case DICT_STATE_EMPTY:
        default: return "empty";
    }
}

static void dict_printSlot(const dict_entry_t *entry, bool pretty) {
    if (entry->key_type == DICT_TYPE_STR)
        printf("  \"%.*s\"", (int)DICT_STR_LENGTH(entry->key), (const char *)entry->key);
    else if (entry->key_type == DICT_TYPE_PTR)
        printf("  \033[36m[%p\033[0m]", entry->key);
    else
        printf("  %ld", (long)DICT_INT(entry->key));

    printf(" \033[1;32m->\033[0m ");

    if (entry->val_type == DICT_TYPE_STR)
        printf("\"%.*s\"", (int)DICT_STR_LENGTH(entry->val), (const char *)entry->val);
    else if (entry->val_type == DICT_TYPE_PTR)
        printf("\033[36m[%p\033[0m]", entry->val);
    else
        printf("%ld", (long)DICT_INT(entry->val));

    if (pretty)
        printf("   [%s -> %s, %s]", dict_typeName(entry->key_type), dict_typeName(entry->val_type),
               dict_stateName(entry->state));
    printf("\n");
}

/* one scalar in the JSON-ish array dump: strings get single quotes, pointers
 * get square brackets, chars get single quotes, everything else is bare. */
static void dict_printScalar(const void *slot, u8 type) {
    switch (type) {
        case DICT_TYPE_STR:
            printf("'%.*s'", (int)DICT_STR_LENGTH(slot), (const char *)slot);
            break;
        case DICT_TYPE_PTR:
            printf("[%p]", (void *)slot);
            break;
        case DICT_TYPE_FLOAT:
            printf("%g", (double)DICT_FLOAT(slot));
            break;
        case DICT_TYPE_DOUBLE:
            printf("%g", DICT_DOUBLE(slot));
            break;
        case DICT_TYPE_CHAR:
            printf("'%c'", DICT_CHAR(slot));
            break;
        case DICT_TYPE_INT:
            printf("%d", DICT_INT(slot));
            break;
        case DICT_TYPE_NONE:
        default:
            printf("None");
            break;
    }
}

/* python-repr shaped dump: braces, one 'key': value pair per line, trailing
 * comma. closer to a json object than the arrow form, but not valid json. */
static void dict_printArray(const dict *self) {
    printf("{\n");
    for (u32 i = 0; i < self->capacity; i++) {
        const dict_entry_t *entry = &self->entries[i];
        if (entry->state != DICT_STATE_USED)
            continue;
        printf("  ");
        dict_printScalar(entry->key, entry->key_type);
        printf(": ");
        dict_printScalar(entry->val, entry->val_type);
        printf(",\n");
    }
    printf("}\n");
}

void dict_print(const dict *self, dict_print_mode_t mode) {
    if (self == NULL || self->entries == NULL)
        return;

    if (mode == DICT_PRINT_HEADING) {
        printf("key -> value\n");
        // return;
    }

    if (mode == DICT_PRINT_ARRAY) {
        dict_printArray(self);
        return;
    }

    bool pretty = mode == DICT_PRINT_PRETTY;
    for (u32 i = 0; i < self->capacity; i++) {
        if (self->entries[i].state != DICT_STATE_USED)
            continue;
        dict_printSlot(&self->entries[i], pretty);
    }
    printf("len %u / capacity %u\n", self->used, self->capacity);
}

#endif

#ifdef __cplusplus
}
#endif
