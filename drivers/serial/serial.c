/*
 * drivers/serial/serial.c — Minimal serial-port (COM1) driver for early debug
 *
 * Uses port I/O to talk to the 8250/16550 UART at 0x3F8 (COM1).
 * This is the single most reliable way to get output out of a kernel
 * before *any* other subsystem comes up.
 */

#include <miniOS/types.h>
#include <asm/io.h>

#define COM1_BASE 0x3F8

/* Serial line status register bits */
#define LSR_TX_EMPTY (1 << 5)
#define LSR_DATA_READY (1 << 0)

void serial_init(void)
{
    /* Disable all interrupts */
    outb(COM1_BASE + 1, 0x00);

    /* Set baud rate divisor to 1 → 115200 baud */
    outb(COM1_BASE + 3, 0x80);    /* Enable DLAB */
    outb(COM1_BASE + 0, 0x01);    /* Divisor low  = 1 */
    outb(COM1_BASE + 1, 0x00);    /* Divisor high = 0 */

    /* 8 data bits, no parity, one stop bit */
    outb(COM1_BASE + 3, 0x03);

    /* Enable FIFO, clear them, 14-byte threshold */
    outb(COM1_BASE + 2, 0xC7);

    /* Disable hardware flow control (RTS/CTS), set DTR+RTS low */
    outb(COM1_BASE + 4, 0x00);
}

static int serial_tx_empty(void)
{
    return inb(COM1_BASE + 5) & LSR_TX_EMPTY;
}

void serial_putchar(char c)
{
    /* Wait for the transmit buffer to be empty */
    while (!serial_tx_empty())
        ;

    outb(COM1_BASE, (u8)c);

    /* CR → CR+LF for terminal friendliness */
    if (c == '\n')
        serial_putchar('\r');
}

void serial_write(const char *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        serial_putchar(data[i]);
}

void serial_puts(const char *str)
{
    while (*str)
        serial_putchar(*str++);
}
