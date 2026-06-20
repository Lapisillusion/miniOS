#ifndef _MINIOS_TYPES_H
#define _MINIOS_TYPES_H

/* Fixed-width integer types */
typedef unsigned char           u8;
typedef unsigned short          u16;
typedef unsigned int            u32;
typedef unsigned long long      u64;

typedef signed char             s8;
typedef signed short            s16;
typedef signed int              s32;
typedef signed long long        s64;

/* Architecture-dependent types (x86_64) */
#ifndef __MINGW64__          /* MinGW provides its own */
typedef unsigned long           size_t;
typedef long                    ssize_t;
typedef unsigned long           uintptr_t;
typedef long                    intptr_t;
typedef long                    ptrdiff_t;
#endif

/* Boolean */
typedef enum {
    false = 0,
    true  = 1
} bool;

/* NULL */
#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* _MINIOS_TYPES_H */
