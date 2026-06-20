#ifndef _MINIOS_PIT_H
#define _MINIOS_PIT_H

#include <miniOS/types.h>

/*
 * pit_init — configure the 8253/8254 PIT to fire IRQ0 at hz Hz.
 *
 * The PIT base clock is 1193180 Hz.  The divisor must be ≤ 65535,
 * so the minimum frequency is ~18.2 Hz and the maximum is 1193180 Hz
 * (divisor = 1).  A typical value for a scheduler is 100 Hz.
 */
void pit_init(u32 hz);

/*
 * g_tick — incremented by the PIT ISR on every fire.
 * Exposed so the scheduler / sleep logic can use it.
 */
extern volatile u64 g_tick;

#endif /* _MINIOS_PIT_H */
