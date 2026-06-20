#ifndef _MINIOS_COMPILER_H
#define _MINIOS_COMPILER_H

/* Branch prediction hints */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* Attributes */
#define __packed        __attribute__((packed))
#define __aligned(n)    __attribute__((aligned(n)))
#define __section(s)    __attribute__((section(s)))
#define __used          __attribute__((used))
#define __noreturn      __attribute__((noreturn))
#define __noinline      __attribute__((noinline))

/* container_of - extract the containing struct from a member pointer */
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* offsetof */
#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif

/* ARRAY_SIZE */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Min/max */
#define min(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a < _b ? _a : _b; \
})
#define max(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a > _b ? _a : _b; \
})

/* Round up to next power of two (alignment) */
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

/* Page helpers */
#define PAGE_SHIFT  12
#define PAGE_SIZE   (1UL << PAGE_SHIFT)
#define PAGE_MASK   (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) ALIGN_UP((x), PAGE_SIZE)

#endif /* _MINIOS_COMPILER_H */
