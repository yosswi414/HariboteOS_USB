#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

#define BUF_SIZ (256)
#define STR_LIM (1024)

char* strcat(char* restrict s1, const char* restrict s2) {
    char* dst = s1;
    const char* src = s2;
    while (*dst) dst++;
    while ((*(dst++) = *(src++))) {}
    return s1;
}
