#ifndef _MINIOS_SERIAL_H
#define _MINIOS_SERIAL_H

#include <miniOS/types.h>

void serial_init(void);
void serial_putchar(char c);
void serial_write(const char *data, size_t size);
void serial_puts(const char *str);

#endif /* _MINIOS_SERIAL_H */
