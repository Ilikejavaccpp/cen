#ifndef CEN__ALIAS_H
#define CEN__ALIAS_H

// File for Aliases
// for various languages and especially the C ENhanced,
// and/or python
#include "ccompare.h"

/* CEN */
/* linux (arch 7.1.15-arch1-2) */
typedef unsigned char sint;
typedef sint u8; // holy C aliases
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
#if (defined(__linux__) || defined(__APPLE__)) &&                              \
    (defined(__clang__) || defined(__GNUC__))
typedef __uint128_t u128;
#else
typedef unsigned long long u128;
#endif

typedef signed char ssint;
typedef signed char i8;
typedef signed short int i16;
typedef signed int i32;
typedef signed long int i64;


typedef float float32;
typedef float f32;
typedef double float64;
typedef double f64;

#ifndef CEN__FLAG_OPTIMIZED
/* don't use this since no SIMD support */
    typedef long double float80;
    typedef long double f80;
#endif

#ifndef CEN__FLAG_OPTIMIZED
#if (defined(__linux__) || defined(__APPLE__)) &&                              \
    (defined(__clang__) || defined(__GNUC__))
    typedef __fp16 float16;
    typedef __fp16 f16;
#else
    typedef unsigned i16;
#endif
#endif


#define CEN__STR_MAX_LEN 2147483647
#define CEN__SSTR_MAX_LEN 65535

/* Python */
typedef void None;

#define print_others(objects) printf("%s", objects)

#define print(objects, sep, end, file, flush)                                  \
    _Generic((objects),                                                        \
      str: str_print(&objects, file),                            \
      str*: str_print(objects, file),                          \
      default: print_others(objects)                                                 \
    );                                                                         \
    fprintf(file, "%s", end);                                                  \
    if (flush) fflush(file);                                                   \

#endif
