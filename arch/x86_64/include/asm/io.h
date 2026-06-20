#ifndef _ASM_IO_H
#define _ASM_IO_H

#include <miniOS/types.h>

/* Port I/O — x86_64 specific */

static inline void outb(u16 port, u8 value)
{
    __asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline u8 inb(u16 port)
{
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(u16 port, u16 value)
{
    __asm__ volatile ("outw %0, %1" :: "a"(value), "Nd"(port));
}

static inline u16 inw(u16 port)
{
    u16 value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outl(u16 port, u32 value)
{
    __asm__ volatile ("outl %0, %1" :: "a"(value), "Nd"(port));
}

static inline u32 inl(u16 port)
{
    u32 value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

#endif /* _ASM_IO_H */
