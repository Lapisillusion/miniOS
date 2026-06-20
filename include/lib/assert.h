#ifndef _MINIOS_ASSERT_H
#define _MINIOS_ASSERT_H

/*
 * kpanic — unrecoverable error.  Prints the message and halts forever.
 */
void kpanic(const char *msg);

/*
 * kassert — if cond is false, print file/line/condition and panic.
 */
#define kassert(cond) \
    do { \
        if (!(cond)) \
            kassert_fail(__FILE__, __LINE__, #cond); \
    } while (0)

/*
 * Internal helper — called by the kassert macro; not for direct use.
 */
void kassert_fail(const char *file, int line, const char *cond);

#endif /* _MINIOS_ASSERT_H */
