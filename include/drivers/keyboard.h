#ifndef _MINIOS_KEYBOARD_H
#define _MINIOS_KEYBOARD_H

#include <miniOS/types.h>

/*
 * keyboard_init — install the IRQ1 handler and unmask the keyboard IRQ.
 *
 * After this call, every keypress fires an interrupt and the scancode
 * is printed to serial / VGA (for now; later we will buffer it for
 * user-space input).
 */
void keyboard_init(void);

/*
 * Returns the last scancode received (0 if the buffer is empty).
 * Non-blocking.
 */
u8 keyboard_get_scancode(void);

#endif /* _MINIOS_KEYBOARD_H */
