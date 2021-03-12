#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

#define BUF_SIZ (256)
#define STR_LIM (1024)

int atoi(const char* str) {
    int len = strlen(str);
    int base = 10;
    int u, r = 0, i = 0;
    char neg = FALSE;
    if (str[0] == '-') neg = TRUE, i++;
    if (str[0] == '0') {
        base = 8, i++;
        if (tolower(str[1]) == 'x') base = 16, i++;
    } else if (tolower(str[len - 1]) == 'b') {
        base = 2;
        len--;
    }
    for (; i < len && isxdigit(str[i]); ++i) {
        if ('a' <= tolower(str[i]) && tolower(str[i]) <= 'f')
            u = tolower(str[i]) - 'a' + 10;
        else
            u = str[i] - '0';
        r = r * base + (neg ? -u : u);
    }
    return r;
}
