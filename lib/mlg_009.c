#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

#define BUF_SIZ (256)
#define STR_LIM (1024)

// put affix like "0x-" in hex or "-b" in bin
// in general no affix would be attached
#define ENABLE_AFFIX FALSE
char* itoa(int value, char* str, int base) {
    int is_neg = FALSE;
    int pos = 0;
    if (base == 10 && value < 0) {
        is_neg = TRUE;
    }
    if (base == 2 && ENABLE_AFFIX) str[pos++] = 'b';
    if (value == 0) str[pos++] = '0';
    while (value != 0) {
        int digit = abs(value % base);
        str[pos++] = (digit > 9 ? digit - 10 + 'a' : digit + '0');
        value /= base;
    }
    if (ENABLE_AFFIX) {
        if (base == 16) str[pos++] = 'x';
        if (base % 8 == 0) str[pos++] = '0';
    }
    if (is_neg) str[pos++] = '-';
    str[pos] = '\0';
    return strrev(str);
}
