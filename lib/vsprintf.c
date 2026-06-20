/*
 * lib/vsprintf.c — Minimal formatted-print engine for the kernel.
 *
 * Supports: %s  %c  %d  %u  %x  %X  %p  %lld  %llu  %llx  %%
 *
 * This is intentionally self-contained — no heap, no floats, no recursion.
 */

#include <miniOS/types.h>
#include <lib/printf.h>

/* ---- helpers ---------------------------------------------------------- */

static int is_digit(char c)  { return c >= '0' && c <= '9'; }

static char *num_to_str_u(unsigned long long val, int base,
                          int width, char pad, char *p)
{
    /* Work backwards from the end of the buffer */
    char *end = p;
    do {
        int d = (int)(val % (unsigned long long)base);
        *--p  = (char)(d < 10 ? '0' + d : 'a' + (d - 10));
        val  /= (unsigned long long)base;
    } while (val != 0);

    while ((end - p) < width)
        *--p = pad;

    return p;
}

static char *num_to_str_s(long long val, int base,
                          int width, char pad, char *p)
{
    int neg = 0;
    if (val < 0) { neg = 1; val = -val; }

    p = num_to_str_u((unsigned long long)val, base, width, pad, p);

    if (neg)
        *--p = '-';

    return p;
}

/* ---- vsnprintf -------------------------------------------------------- */

int vsnprintf(char *buf, size_t size, const char *fmt,
              __builtin_va_list args)
{
    char *dst    = buf;
    char *end    = buf + size - 1;   /* leave room for NUL */
    char *cursor = buf;

    if (size == 0)
        return 0;

    while (*fmt) {
        if (*fmt != '%') {
            if (cursor < end) *cursor++ = *fmt;
            fmt++;
            continue;
        }

        fmt++; /* skip '%' */

        /* Parse optional width */
        int width = 0;
        char pad  = ' ';
        if (*fmt == '0') { pad = '0'; fmt++; }
        while (is_digit(*fmt)) {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        /* Parse length modifier */
        int is_longlong = 0;
        if (*fmt == 'l') {
            fmt++;
            if (*fmt == 'l') {
                is_longlong = 1;
                fmt++;
            }
        }

        /* ---- format specifier ---- */
        switch (*fmt) {

        case 's': {
            const char *s = __builtin_va_arg(args, const char *);
            if (s == NULL) s = "(null)";
            while (*s && cursor < end)
                *cursor++ = *s++;
            break;
        }

        case 'c': {
            char c = (char)__builtin_va_arg(args, int);
            if (cursor < end) *cursor++ = c;
            break;
        }

        case 'd':
        case 'i': {
            char tmp[32];
            char *p   = tmp + sizeof(tmp);
            long long v;

            if (is_longlong)
                v = __builtin_va_arg(args, long long);
            else
                v = (long long)__builtin_va_arg(args, int);

            p = num_to_str_s(v, 10, width, pad, p);
            while (*p && cursor < end) *cursor++ = *p++;
            break;
        }

        case 'u': {
            char tmp[32];
            char *p = tmp + sizeof(tmp);
            unsigned long long v;

            if (is_longlong)
                v = __builtin_va_arg(args, unsigned long long);
            else
                v = (unsigned long long)__builtin_va_arg(args, unsigned int);

            p = num_to_str_u(v, 10, width, pad, p);
            while (*p && cursor < end) *cursor++ = *p++;
            break;
        }

        case 'X':
        case 'x': {
            char tmp[32];
            char *p   = tmp + sizeof(tmp);
            unsigned long long v;

            if (is_longlong)
                v = __builtin_va_arg(args, unsigned long long);
            else
                v = (unsigned long long)__builtin_va_arg(args, unsigned int);

            p = num_to_str_u(v, 16, width, pad, p);

            /* Upper-case hex */
            if (*fmt == 'X')
                for (char *q = p; *q; q++)
                    if (*q >= 'a' && *q <= 'f')
                        *q = (char)(*q - 'a' + 'A');

            while (*p && cursor < end) *cursor++ = *p++;
            break;
        }

        case 'p': {
            void *ptr = __builtin_va_arg(args, void *);
            if (cursor + 1 < end) { *cursor++ = '0'; *cursor++ = 'x'; }
            char tmp[32];
            char *p = num_to_str_u((unsigned long long)(uintptr_t)ptr,
                                   16, (int)(sizeof(void *) * 2), '0',
                                   tmp + sizeof(tmp));
            while (*p && cursor < end) *cursor++ = *p++;
            break;
        }

        case '%':
            if (cursor < end) *cursor++ = '%';
            break;

        default:
            /* Unknown format — print it literally */
            if (cursor + 1 < end) { *cursor++ = '%'; *cursor++ = *fmt; }
            break;
        }

        fmt++;
    }

    *cursor = '\0';
    return (int)(cursor - dst);
}
