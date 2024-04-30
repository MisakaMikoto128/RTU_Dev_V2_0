#include <assert.h>
/**
  Called from assert() and prints a message on stderr and calls abort().

  \param[in] expr  assert expression that was not TRUE
  \param[in] file  source file of the assertion
  \param[in] line  source line of the assertion
*/
__attribute__((noreturn)) void __aeabi_assert(const char *expr, const char *file, int line)
{
    char str[12], *p;

    // SEGGER_RTT_printf(0, "*** assertion failed: \r\n");
    // SEGGER_RTT_printf(0, expr);
    // SEGGER_RTT_printf(0, "\r\n");
    // SEGGER_RTT_printf(0, ", file \r\n");
    // SEGGER_RTT_printf(0, file);
    // SEGGER_RTT_printf(0, "\r\n");
    // SEGGER_RTT_printf(0, ", line \r\n");

    // p = str + sizeof(str);
    // *--p = '\0';
    // *--p = '\n';
    // while (line > 0)
    // {
    //     *--p = '0' + (line % 10);
    //     line /= 10;
    // }
    // SEGGER_RTT_printf(0, p);
    // SEGGER_RTT_printf(0, "\r\n");
    abort();
}

void abort(void)
{
    for (;;);
}