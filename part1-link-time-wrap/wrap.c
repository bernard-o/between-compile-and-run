/* A link-time wrapper for printf, enabled with -Wl,--wrap=printf.
 * Every call to printf is redirected here; __real_printf reaches the original. */
#include <stdio.h>
#include <stdarg.h>

extern int __real_printf(const char *fmt, ...);   /* provided by --wrap */

static int call_count = 0;

int __wrap_printf(const char *fmt, ...) {
    call_count++;
    __real_printf("[printf #%d] ", call_count);    /* our instrumentation */
    va_list ap;
    va_start(ap, fmt);
    int r = vprintf(fmt, ap);                       /* the real output */
    va_end(ap);
    return r;
}
