#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

#define BUF_SIZ (256)
#define STR_LIM (1024)

void* memcpy(void* restrict s1, const void* restrict s2, size_t n) {
    unsigned char* dst = s1;
    const unsigned char* src = s2;
    while (n--) *(dst++) = *(src++);
    return s1;
}
