; arch/x86_64/boot/bios/stage2.s
; ---------------------------------------------------------------------------
; Stage 2: Real-mode loader → 32-bit protected mode → 64-bit long mode
;
; Loaded by the MBR to 0x1000.
;  1. Load the flat kernel binary from disk to 0x100000 (1 MiB)
;  2. Query the BIOS memory map  (int 0x15 E820)
;  3. Enable A20
;  4. Build identity-mapped page tables
;  5. Switch: real → protected (32-bit) → long (64-bit)
;  6. Fill boot_info_t and jump to the kernel
; ---------------------------------------------------------------------------

        [ORG 0x1000]
        [BITS 16]

; ---------------------------------------------------------------------------
; Constants
; ---------------------------------------------------------------------------
KERNEL_LBA       equ 9               ; kernel starts at LBA 9
KERNEL_PHYS      equ 0x100000        ; final kernel location (1 MiB)
KERNEL_LOAD_TMP  equ 0x7E00          ; staging area in low memory

; KERNEL_SECTORS is set by the Makefile via NASM -d flag.
; Fallback for manual assembly:
%ifndef KERNEL_SECTORS
KERNEL_SECTORS   equ 32
%endif

PML4_ADDR        equ 0xB000          ; PML4 page table
PDPT_ADDR        equ 0xC000          ; Page-Directory-Pointer Table
PD_ADDR          equ 0xD000          ; Page Directory

PAGE_PRESENT     equ (1 << 0)
PAGE_WRITABLE    equ (1 << 1)
PAGE_HUGE        equ (1 << 7)        ; 2 MiB huge page (PD level)

E820_BUF_SEG     equ 0x0000
E820_BUF_OFF     equ 0x6000          ; store up to 64 entries here

SEG_CODE64       equ 0x08
SEG_DATA         equ 0x10

; ---------------------------------------------------------------------------
; Entry point — still in real mode
; ---------------------------------------------------------------------------
stage2_entry:
        ; Clear segments and set up a stack below this code
        xor     ax, ax
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, 0x1000          ; stack grows down from stage2 base

        ; Save boot drive number (passed from MBR in DL)
        mov     [boot_drive], dl

        ; Print stage2 banner
        mov     si, msg_stage2
        call    print16

        ; ---------------------------------------------------------------
        ; 1.  Enable A20 FIRST — loading the kernel to 1 MiB requires it
        ; ---------------------------------------------------------------
        call    enable_a20

        ; ---------------------------------------------------------------
        ; 2.  Load the kernel from disk  (LBA → 1 MiB)
        ; ---------------------------------------------------------------
        mov     si, msg_load
        call    print16

        mov     si, dap_kernel
        mov     ah, 0x42
        mov     dl, [boot_drive]
        int     0x13

        jnc     .load_ok

        mov     si, msg_disk_err
        call    print16
        hlt
        jmp     $

.load_ok:
        ; ---------------------------------------------------------------
        ; 3.  BIOS E820 memory map
        ; ---------------------------------------------------------------
        mov     si, msg_e820
        call    print16

        xor     ebx, ebx                    ; ebx = continuation value
        mov     edi, E820_BUF_OFF           ; where to write entries
        xor     bp, bp                      ; bp = entry counter (max 128)

.e820_loop:
        mov     eax, 0xE820
        mov     ecx, 20                     ; ask for 20-byte entries
        mov     edx, 0x534D4150             ; 'SMAP'
        int     0x15
        jc      .e820_done
        cmp     eax, 0x534D4150             ; verify SMAP
        jne     .e820_done
        cmp     ecx, 20
        jl      .e820_done                  ; entry too short → done
        add     edi, 20                     ; advance buffer pointer
        inc     bp
        cmp     ebx, 0
        jne     .e820_loop
.e820_done:
        mov     [e820_count], bp

        ; ---------------------------------------------------------------
        ; 4.  Build identity-mapped page tables
        ;     PML4[0] → PDPT  |  PDPT[0] → PD  |  PD[i] → 2MiB huge pages
        ; ---------------------------------------------------------------
        call    build_pages

        ; ---------------------------------------------------------------
        ; 5.  Load GDT, enter protected mode (32-bit)
        ; ---------------------------------------------------------------
        cli
        lgdt    [gdt32_ptr]

        mov     eax, cr0
        or      eax, 1                      ; set CR0.PE
        mov     cr0, eax

        ; Far jump to flush prefetch queue → 32-bit code
        jmp     0x08:protected_mode_entry

        ; ---------------------------------------------------------------
        ; 6.  32-bit → 64-bit long mode
        ; ---------------------------------------------------------------
        [BITS 32]
protected_mode_entry:
        ; Reload data segments
        mov     ax, 0x10
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        mov     ss, ax
        mov     esp, 0x10000               ; 32-bit stack

        ; ---------------------------------------------------------------
        ; Copy kernel from staging area (0x7E00) to 1 MiB (0x100000)
        ; ---------------------------------------------------------------
        mov     esi, KERNEL_LOAD_TMP       ; source: low memory
        mov     edi, KERNEL_PHYS           ; dest:   1 MiB
        mov     ecx, KERNEL_SECTORS
        shl     ecx, 7                     ; sectors * 512 / 4 = sectors * 128 dwords
        rep movsd

        ; Load PML4
        mov     eax, PML4_ADDR
        mov     cr3, eax

        ; Enable PAE
        mov     eax, cr4
        or      eax, 0x20
        mov     cr4, eax

        ; Enable Long Mode (EFER.LME)
        mov     ecx, 0xC0000080
        rdmsr
        or      eax, 0x100
        wrmsr

        ; Enable Paging (CR0.PG)
        mov     eax, cr0
        or      eax, 0x80000000
        mov     cr0, eax

        ; Load 64-bit GDT and far-jump into long mode
        lgdt    [gdt64_ptr]
        jmp     SEG_CODE64:long_mode_entry

        ; ---------------------------------------------------------------
        ; 7.  64-bit kernel hand-off
        ; ---------------------------------------------------------------
        [BITS 64]
long_mode_entry:
        ; Reload data segments
        mov     ax, SEG_DATA
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        mov     ss, ax

        ; Pass E820 buffer and count to the kernel
        ;   rdi = E820 buffer address
        ;   rsi = E820 entry count
        mov     edi, E820_BUF_OFF
        movzx   esi, word [e820_count]
        
        ; Jump to the kernel (flat binary, first byte = 64-bit entry)
        mov     rax, KERNEL_PHYS
        jmp     rax

; ======================================================================
; Subroutines  (real mode)
; ======================================================================
        [BITS 16]

; -----------------------------------------------------------------------
; enable_a20 — enable A20 address line via keyboard controller
; -----------------------------------------------------------------------
enable_a20:
        cli
        call    .wait_in
        mov     al, 0xAD                ; disable keyboard
        out     0x64, al
        call    .wait_in
        mov     al, 0xD0                ; read output port
        out     0x64, al
        call    .wait_out
        in      al, 0x60
        push    ax
        call    .wait_in
        mov     al, 0xD1                ; write output port
        out     0x64, al
        call    .wait_in
        pop     ax
        or      al, 2                   ; set A20 bit
        out     0x60, al
        call    .wait_in
        mov     al, 0xAE                ; enable keyboard
        out     0x64, al
        call    .wait_in
        sti
        ret

.wait_in:
        in      al, 0x64
        test    al, 2                   ; input buffer full?
        jnz     .wait_in
        ret

.wait_out:
        in      al, 0x64
        test    al, 1                   ; output buffer full?
        jz      .wait_out
        ret

; -----------------------------------------------------------------------
; build_pages — identity-map 512 × 2 MiB = 1 GiB
;   PML4[0] → PDPT  (present | writable)
;   PDPT[0] → PD     (present | writable)
;   PD[0..511]        (present | writable | huge, phys = i × 2MiB)
; -----------------------------------------------------------------------
build_pages:
        ; Clear PML4
        mov     di, PML4_ADDR
        xor     ax, ax
        mov     cx, 2048                 ; 4096 bytes → 2048 words
        rep stosw

        ; Clear PDPT
        mov     di, PDPT_ADDR
        mov     cx, 2048
        rep stosw

        ; Clear PD
        mov     di, PD_ADDR
        mov     cx, 2048
        rep stosw

        ; PML4[0] = PDPT | 0x3
        mov     eax, PDPT_ADDR
        or      eax, PAGE_PRESENT | PAGE_WRITABLE
        mov     [PML4_ADDR], eax

        ; PDPT[0] = PD | 0x3
        mov     eax, PD_ADDR
        or      eax, PAGE_PRESENT | PAGE_WRITABLE
        mov     [PDPT_ADDR], eax

        ; Fill PD[0..511]
        mov     di, PD_ADDR
        xor     edx, edx                ; edx = 2 MiB index
.pd_fill:
        mov     eax, edx
        shl     eax, 21                 ; addr = i * 2 MiB
        or      eax, PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE
        mov     [di], eax               ; low 32 bits
        mov     dword [di + 4], 0       ; high 32 bits = 0
        add     di, 8
        inc     edx
        cmp     edx, 512
        jb      .pd_fill
        ret

; -----------------------------------------------------------------------
; print16 — real-mode BIOS tty output
;   IN: DS:SI → NUL-terminated string
; -----------------------------------------------------------------------
print16:
        lodsb
        or      al, al
        jz      .done
        mov     ah, 0x0E
        mov     bx, 0x0007
        int     0x10
        jmp     print16
.done:
        ret

; -----------------------------------------------------------------------
; Data

; -----------------------------------------------------------------------
; Data
; -----------------------------------------------------------------------
msg_stage2:   db 13, 10, "stage2", 0
msg_load:     db " load", 0
msg_e820:     db " e820", 0
msg_disk_err: db " err", 0
boot_drive:   db 0
msg_stage2_finish:   db "test finish", 0

e820_count:   dw 0

; Disk Address Packet — kernel
; Load to a safe low-memory address first (0x07E0:0x0000 = phys 0x7E00).
; The kernel is later copied to 1 MiB in 32-bit protected mode, where we
; have full 32-bit addressing and don't need segment:offset tricks.
dap_kernel:
        db      0x10                    ; DAP size
        db      0                       ; reserved
        dw      KERNEL_SECTORS          ; sector count
        dw      0                       ; buffer offset (= 0x0000)
        dw      0x07E0                  ; buffer segment (= 0x7E00 phys)
        dq      KERNEL_LBA              ; starting LBA

; -----------------------------------------------------------------------
; 32-bit GDT
; -----------------------------------------------------------------------
        align 8
gdt32:
        dq 0x0000000000000000           ; null
        dq 0x00CF9A000000FFFF           ; 32-bit code: base=0, limit=4G, DPL=0
        dq 0x00CF92000000FFFF           ; 32-bit data: base=0, limit=4G, DPL=0
gdt32_end:

gdt32_ptr:
        dw gdt32_end - gdt32 - 1
        dd gdt32

; -----------------------------------------------------------------------
; 64-bit GDT
; -----------------------------------------------------------------------
        align 8
gdt64:
        dq 0x0000000000000000           ; null
        dq 0x00AF98000000FFFF           ; 64-bit code (L=1, D=0, P=1, DPL=0)
        dq 0x00CF92000000FFFF           ; 64-bit data
gdt64_end:

gdt64_ptr:
        dw gdt64_end - gdt64 - 1
        dq gdt64

        times 4096 - ($ - $$) db 0       ; pad to exactly 4096 bytes
