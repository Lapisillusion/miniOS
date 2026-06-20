#ifndef _MINIOS_KERNEL_H
#define _MINIOS_KERNEL_H

#include <miniOS/types.h>

/* Kernel panic - never returns */
void kpanic(const char *msg);

#endif /* _MINIOS_KERNEL_H */
