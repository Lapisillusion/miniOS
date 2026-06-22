/*
 * arch/x86_64/include/asm/paging.h — x86_64 page-table types and constants
 *
 * Defines the hardware format of 4-level paging entries and the macros
 * needed to split a virtual address into PML4/PDPT/PD/PT indices.
 *
 * Phase 4 (PMM) doesn't manipulate page tables yet — this header exists
 * as a reference for PAGE_* flag definitions and to prepare for Phase 5.
 */

#ifndef _ASM_PAGING_H
#define _ASM_PAGING_H

#include <miniOS/types.h>

/* ---- 4-level paging indices -------------------------------------------- */

#define PML4_INDEX(va)  (((u64)(va) >> 39) & 0x1FF)
#define PDPT_INDEX(va)  (((u64)(va) >> 30) & 0x1FF)
#define PD_INDEX(va)    (((u64)(va) >> 21) & 0x1FF)
#define PT_INDEX(va)    (((u64)(va) >> 12) & 0x1FF)

/* ---- Page-table entry flags --------------------------------------------- */

#define PAGE_PRESENT    (1ULL << 0)   /* Page is in memory               */
#define PAGE_WRITE      (1ULL << 1)   /* Read/write (0 = read-only)      */
#define PAGE_USER       (1ULL << 2)   /* User (0 = supervisor)           */
#define PAGE_PWT        (1ULL << 3)   /* Page-level write-through         */
#define PAGE_PCD        (1ULL << 4)   /* Page-level cache disable         */
#define PAGE_ACCESSED   (1ULL << 5)   /* Set by CPU on access            */
#define PAGE_DIRTY      (1ULL << 6)   /* Set by CPU on write             */
#define PAGE_HUGE       (1ULL << 7)   /* 2 MiB (PDE) / 1 GiB (PDPTE)    */
#define PAGE_GLOBAL     (1ULL << 8)   /* Global page (ignored TLB flush) */
#define PAGE_NO_EXEC    (1ULL << 63)  /* Execute-disable (NX)            */

/* Composite flags */
#define PAGE_KERNEL_RW  (PAGE_PRESENT | PAGE_WRITE)
#define PAGE_KERNEL_RO  (PAGE_PRESENT)
#define PAGE_USER_RW    (PAGE_PRESENT | PAGE_WRITE | PAGE_USER)
#define PAGE_USER_RO    (PAGE_PRESENT | PAGE_USER)

/* Address mask — extracts the physical page-frame address [51:12] */
#define PAGE_ADDR_MASK  0x000FFFFFFFFFF000ULL

/* ---- Entry types (union: raw u64 or bit-fields) ------------------------ */

typedef union {
    u64 raw;
    struct {
        u64 present    : 1;   /* bit  0: page present               */
        u64 write      : 1;   /* bit  1: read/write                 */
        u64 user       : 1;   /* bit  2: user/supervisor            */
        u64 pwt        : 1;   /* bit  3: write-through              */
        u64 pcd        : 1;   /* bit  4: cache disable              */
        u64 accessed   : 1;   /* bit  5: accessed flag              */
        u64 dirty      : 1;   /* bit  6: dirty flag                 */
        u64 page_size  : 1;   /* bit  7: PAT (PTE) / PS (PDE,PDPTE)*/
        u64 global     : 1;   /* bit  8: global page                */
        u64 avail      : 3;   /* bits 9–11: OS-available            */
        u64 addr       : 40;  /* bits 12–51: physical page frame    */
        u64 available  : 11;  /* bits 52–62: OS-available           */
        u64 no_exec    : 1;   /* bit 63: execute-disable            */
    } __packed;
} pml4e_t, pdpte_t, pde_t, pte_t;

/* Compile-time size check — each entry must be exactly 8 bytes */
_Static_assert(sizeof(pml4e_t) == 8,  "pml4e_t must be 8 bytes");
_Static_assert(sizeof(pdpte_t) == 8,  "pdpte_t must be 8 bytes");
_Static_assert(sizeof(pde_t)   == 8,  "pde_t must be 8 bytes");
_Static_assert(sizeof(pte_t)   == 8,  "pte_t must be 8 bytes");

#endif /* _ASM_PAGING_H */
