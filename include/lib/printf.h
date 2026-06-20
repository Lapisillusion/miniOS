#ifndef _MINIOS_PRINTF_H
#define _MINIOS_PRINTF_H

#include <miniOS/types.h>

/*
 * kprintf — formatted print to both serial and VGA.
 * Supports: %s %c %d %u %x %X %p %lld %llu %llx %%
 */
void kprintf(const char *fmt, ...);

/*
 * kvprintf — va_list variant for internal use.
 */
void kvprintf(const char *fmt, __builtin_va_list args);

/*
 * kputs — print a plain string to both serial and VGA.
 */
void kputs(const char *str);

/*
 * vsnprintf — format into a buffer.  Returns the number of characters
 * that *would* have been written (excluding NUL), even if truncated.
 */
int vsnprintf(char *buf, size_t size, const char *fmt, __builtin_va_list args);

#endif /* _MINIOS_PRINTF_H */
