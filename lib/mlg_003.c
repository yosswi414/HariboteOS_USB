#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

#define BUF_SIZ (256)
#define STR_LIM (1024)

char* strncpy(char* restrict s1, const char* restrict s2, size_t n) {
    int pos = 0;
    char ch;
    do {
        ch = s2[pos];
        s1[pos++] = ch;
        if (ch == '\0') break;
    } while (pos < n);
    return s1;
}
