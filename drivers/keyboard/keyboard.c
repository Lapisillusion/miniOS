/*
 * drivers/keyboard/keyboard.c — PS/2 keyboard driver (IRQ1)
 *
 * Scancode-set-1 (legacy XT) decoder.  Every keypress / release fires
 * IRQ1; we read the scancode from port 0x60 and, for now, just echo
 * press events to serial / VGA.
 */

#include <drivers/keyboard.h>
#include <asm/io.h>
#include <lib/printf.h>

#define KB_DATA      0x60

/* ---- Forward declarations from irq.c -------------------------------- */
extern void irq_register(u8 irq, void (*handler)(void));
extern void irq_unmask(u8 irq);

/* ---- Scancode set 1 → ASCII  (lower-case only, no modifiers yet) --- */

static const char scancode_ascii[128] = {
    /* 0x00 */ 0,   0,   '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    /* 0x10 */ '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    /* 0x20 */ 'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    /* 0x30 */ 'z','x','c','v','b','n','m',',','.','/',0,  0,  0,  ' ',
    /* 0x40-0x7F */ 0,
};

static u8 g_last_scancode = 0;

/* ---- kb_handler — IRQ1 ISR ----------------------------------------- */
static void kb_handler(void)
{
    u8 code = inb(KB_DATA);           /* read the scancode */

    /* Bit 7 set → key released; ignore for now */
    if ((code & 0x80) == 0) {
        g_last_scancode = code;

        char c = scancode_ascii[code];
        if (c >= ' ') {
            kprintf("%c", c);
        } else if (c == '\n') {
            kprintf("\n");
        } else if (c == '\b') {
            kprintf("\b");
        }
    }
}

/* ---- keyboard_get_scancode ------------------------------------------ */
u8 keyboard_get_scancode(void)
{
    u8 tmp = g_last_scancode;
    g_last_scancode = 0;
    return tmp;
}

/* ---- keyboard_init -------------------------------------------------- */
void keyboard_init(void)
{
    irq_register(1, kb_handler);
    irq_unmask(1);

    kprintf("  kbd     : PS/2 IRQ1 installed\n");
}
