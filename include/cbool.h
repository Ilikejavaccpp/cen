#ifndef STDCEN_BOOL_H
#define STDCEN_BOOL_H

#ifdef __cplusplus
using boolean = bool;
#else
#ifndef __bool_true_false_are_defined
  #ifndef bool
    typedef _Bool bool;
    typedef bool boolean;
    #define true 1
    #define false 0
  #endif
#endif


#endif

#endif
