; arch/x86_64/boot/bios/mbr.s
; ---------------------------------------------------------------------------
; Stage 1: Master Boot Record (exactly 512 bytes)
;
; BIOS loads this to 0x7C00 in real mode.  Its only job: load stage2 from
; disk sectors into memory, then jump to it.
;
; Stage2 sectors are written immediately after this sector on disk.
; ---------------------------------------------------------------------------

        [ORG 0x7C00]
        [BITS 16]

STAGE2_LBA       equ 1               ; stage2 starts at LBA 1
STAGE2_SECTORS   equ 8               ; 8 sectors = 4 KiB (plenty)
STAGE2_SEGMENT   equ 0x0000
STAGE2_OFFSET    equ 0x1000          ; load to 0x0000:0x1000

mbr_start:
        ; Normalise CS:IP to 0x0000:0x7C00
        jmp     0x0000:.init

.init:
        ; Segments to zero
        xor     ax, ax
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, 0x7C00          ; stack grows down from MBR

        ; Save boot drive number (BIOS passes in DL)
        mov     [boot_drive], dl

        ; Show that we are alive
        mov     si, msg_mbr
        call    print

        ; ---------------------------------------------------------------
        ; Load stage2 using extended read (int 0x13 AH=0x42)
        ; ---------------------------------------------------------------
        mov     si, dap             ; DS:SI → Disk Address Packet
        mov     ah, 0x42
        mov     dl, [boot_drive]
        int     0x13
        jc      .disk_err           ; carry set → read failed

        ; Jump to stage2
        jmp     STAGE2_SEGMENT:STAGE2_OFFSET

.disk_err:
        mov     si, msg_err
        call    print
        hlt
        jmp     .disk_err

; ---------------------------------------------------------------------------
; print — write a NUL-terminated string to the screen via BIOS tty
;   IN: DS:SI → string
; ---------------------------------------------------------------------------
print:
        lodsb
        or      al, al
        jz      .done
        mov     ah, 0x0E
        mov     bx, 0x0007          ; page 0, light grey
        int     0x10
        jmp     print
.done:
        ret

; ---------------------------------------------------------------------------
; Data
; ---------------------------------------------------------------------------
msg_mbr: db      "MBR", 0
msg_err: db      "!", 0

boot_drive:     db 0

; Disk Address Packet (DAP) for extended read
dap:
        db      0x10                ; DAP size  = 16 bytes
        db      0                   ; reserved
        dw      STAGE2_SECTORS      ; sector count
        dw      STAGE2_OFFSET       ; buffer offset
        dw      STAGE2_SEGMENT      ; buffer segment
        dq      STAGE2_LBA          ; starting LBA

; ---------------------------------------------------------------------------
; Pad to 510 bytes, then boot signature
; ---------------------------------------------------------------------------
        times 510 - ($ - $$) db 0
        dw      0xAA55
