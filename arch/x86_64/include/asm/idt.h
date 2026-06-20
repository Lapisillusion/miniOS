#ifndef _ASM_IDT_H
#define _ASM_IDT_H

#include <miniOS/types.h>
#include <miniOS/compiler.h>

/*
 * 64-bit IDT gate descriptor — 16 bytes, exactly as the CPU expects.
 */
typedef struct {
    u16 offset_low;      /* ISR address [ 0:15] */
    u16 segment;         /* code segment selector (0x08) */
    u8  ist;             /* bits 0-2: IST index; bits 3-7: 0 */
    u8  type;            /* 0x8E = interrupt gate, 0x8F = trap gate */
    u16 offset_mid;      /* ISR address [16:31] */
    u32 offset_high;     /* ISR address [32:63] */
    u32 __reserved;      /* must be 0 */
} __packed idt_entry_t;

/*
 * IDT pointer passed to lidt.
 */
typedef struct {
    u16 limit;
    u64 base;
} __packed idt_ptr_t;

/* ------------------------------------------------------------------ */
/* Public API                                                        */
/* ------------------------------------------------------------------ */

void idt_init(void);
void idt_set_gate(u8 vector, void *isr, u8 type);

#endif /* _ASM_IDT_H */
