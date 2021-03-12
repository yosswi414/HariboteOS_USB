#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

#define BUF_SIZ (256)
#define STR_LIM (1024)

int memcmp(const void* restrict buf1, const void* restrict buf2, size_t n) {
    const unsigned char* p1 = buf1;
    const unsigned char* p2 = buf2;
    for (size_t i = 0; i < n; ++i, ++p1, ++p2)
        if (*p1 != *p2) return 1 - 2 * (*p1 < *p2);
    return 0;
}