#include <drivers/keyboard.h>
#include <drivers/serial.h>
#include <asm/io.h>
#include <lib/printf.h>

#define KB_DATA 0x60

extern void irq_register(u8 irq, void (*handler)(void));
extern void irq_unmask(u8 irq);

static const u8 scancode_set1[128] = {
    [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',[0x07]='6',
    [0x08]='7',[0x09]='8',[0x0A]='9',[0x0B]='0',[0x0C]='-',[0x0D]='=',
    [0x0E]='\b',[0x0F]='\t',[0x10]='q',[0x11]='w',[0x12]='e',[0x13]='r',
    [0x14]='t',[0x15]='y',[0x16]='u',[0x17]='i',[0x18]='o',[0x19]='p',
    [0x1A]='[',[0x1B]=']',[0x1C]='\n',[0x1E]='a',[0x1F]='s',[0x20]='d',
    [0x21]='f',[0x22]='g',[0x23]='h',[0x24]='j',[0x25]='k',[0x26]='l',
    [0x27]=';',[0x28]='\'',[0x29]='`',[0x2B]='\\',[0x2C]='z',[0x2D]='x',
    [0x2E]='c',[0x2F]='v',[0x30]='b',[0x31]='n',[0x32]='m',[0x33]=',',
    [0x34]='.',[0x35]='/',[0x39]=' ',
};

static u8 g_last_scancode = 0;

static void kb_handler(void)
{
    /* Ignore mouse data (PS/2 status bit 5) — QEMU has a mouse by default */
    if (inb(0x64) & 0x20)
        return;

    u8 code = inb(KB_DATA);
    if (!(code & 0x80) && scancode_set1[code]) {
        g_last_scancode = code;
        char c = (char)scancode_set1[code];
        if (c == '\n') kprintf("\n");
        else if (c == '\b') kprintf("\b");
        else kprintf("%c", c);
    }
}

u8 keyboard_get_scancode(void) { u8 t = g_last_scancode; g_last_scancode = 0; return t; }

static void kb_wait_out(void) { while (!(inb(0x64) & 1)) ; }
static void kb_wait_in(void)  { while (inb(0x64) & 2) ; }

void keyboard_init(void)
{
    u8 cfg;

    /* Dump any pending keyboard data before we start */
    while (inb(0x64) & 1) inb(0x60);

    /* Ensure PS/2 controller scancode translation is ON.
     * stage2's A20 code may have left it off → scancode set 2 → garbled. */
    kb_wait_in();  outb(0x64, 0x20);          /* read config byte */
    kb_wait_out(); cfg = inb(0x60);
    cfg |= 0x40;                               /* set bit 6: translation */
    kb_wait_in();  outb(0x64, 0x60);          /* write config byte */
    kb_wait_in();  outb(0x60, cfg);

    while (inb(0x64) & 1) inb(0x60);          /* flush after config write */

    /* Enable scanning */
    kb_wait_in();  outb(0x60, 0xF4);
    kb_wait_out(); inb(0x60);                  /* ACK */

    while (inb(0x64) & 1) inb(0x60);          /* final flush */

    irq_register(1, kb_handler);
    irq_unmask(1);
    kprintf("  kbd     : PS/2 IRQ1 installed\n");
}
