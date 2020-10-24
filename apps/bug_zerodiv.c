#include "api.h"

void HariMain(void) {
    volatile int a = 24, b = 3, c = 0;
    c = a / b;
    api_putstr0("24 / 3 = ");
    api_putchar(c + '0');
    api_putstr0("\n24 / 0 = ");
    b = 0;
    c = a / b;
    api_putchar(c + '0');
    api_putchar('\n');
    api_end();
}