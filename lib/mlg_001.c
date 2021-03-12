#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

#define BUF_SIZ (256)
#define STR_LIM (1024)

size_t strlen(const char* s) {
    int len = 0;
    while (s[len] != '\0' && len <= STR_LIM) ++len;
    if (STR_LIM < len) return STR_LIM;
    return len;
}
